//////////////////////////////////////////////////////////////////////////
// Matt.Dobbs@Cern.CH, September 1999
// Updated: 7.1.2000 iterators complete and working!
// Updated: 10.1.2000 GenEvent::vertex, particle iterators are made 
//                    constant WRT this event ... note that 
//                    GenVertex::***_iterator is not const, since it must
//                    be able to return a mutable pointer to itself.
// Updated: 08.2.2000 the event now holds a set of all attached vertices
//                    rather than just the roots of the graph
// Event record for MC generators (for use at any stage of generation)
//////////////////////////////////////////////////////////////////////////

#include "CLHEP/HepMC/GenEvent.h"
#include <stdio.h>       // needed for formatted output using sprintf 

namespace HepMC {

    GenEvent::GenEvent( int signal_process_id, int event_number,
			GenVertex* signal_vertex,
			const WeightContainer& weights,
			const std::vector<double>& random_states ) :
	m_signal_process_id(signal_process_id), m_event_number(event_number),
	m_event_scale(-1), m_alphaQCD(-1), m_alphaQED(-1),
	m_signal_process_vertex(signal_vertex), m_weights(weights),
	m_random_states(random_states)
    {
	// note: default values for m_event_scale, m_alphaQCD, m_alphaQED
	//       are as suggested in hep-ph/0109068, "Generic Interface..."
	s_counter++;
    }

    GenEvent::GenEvent( const GenEvent& inevent ) 
    {
	// deep copy
	*this = inevent;
	s_counter++;
    }

    GenEvent::~GenEvent() 
    {
	// Deep destructor.
	// deletes all vertices/particles in this evt
	//
	delete_all_vertices();
	s_counter--;
    }

    GenEvent& GenEvent::operator=( const GenEvent& inevent ) 
    {
	// deep - makes a copy of all vertices!
	//
	// 1. Delete all vertices attached to this
	delete_all_vertices();
	//    
	// 2. create a NEW copy of all vertices from inevent
	//    taking care to map new vertices onto the vertices being copied
	//    and add these new vertices to this event.
	std::map<const GenVertex*,GenVertex*> map_in_to_new;
	for ( GenEvent::vertex_const_iterator v = inevent.vertices_begin();
	      v != inevent.vertices_end(); v++ ) {
	    GenVertex* newvertex = new GenVertex(**v);
	    map_in_to_new[*v] = newvertex;
	    add_vertex( newvertex );
	}
	// 2.b copy the signal process vertex info.
	if ( inevent.signal_process_vertex() ) {
	    set_signal_process_vertex( 
		map_in_to_new[inevent.signal_process_vertex()] );
	} else set_signal_process_vertex( 0 );
	//
	// 3. for each outgoing particle in each vtx that had a decay vertex
	//    in the old list, set the outparticle.decayvertex pointers to 
	//    the right place in the new list ...
	for ( std::map<const GenVertex*,GenVertex*>::const_iterator m = 
		  map_in_to_new.begin();
	      m != map_in_to_new.end(); m++ ) {
	    for ( GenVertex::particles_out_const_iterator 
		      pold = ((*m).first)->particles_out_const_begin(),
		      pnew = ((*m).second)->particles_out_const_begin();
		  pold != ((*m).first)->particles_out_const_end(); 
		  ++pold, ++pnew ) {
		if (  (*pold)->end_vertex() ) {
		    map_in_to_new[(*pold)->end_vertex()]->
			add_particle_in(*pnew);
		}
	    }
	}
	//
	// 4. now that vtx/particles are copied, do everything else
	set_signal_process_id( inevent.signal_process_id() );
	set_event_number( inevent.event_number() );
	set_event_scale( inevent.event_scale() );
	set_alphaQCD( inevent.alphaQCD() );
	set_alphaQED( inevent.alphaQED() );
	set_random_states( inevent.random_states() );
	weights() = inevent.weights();
	return *this;
    }

    void GenEvent::print( std::ostream& ostr ) const {
	// dumps the content of this event to ostr
	//   to dump to cout use: event.print();
	//   if you want to write this event to file outfile.txt you could use:
	//      std::ofstream outfile("outfile.txt"); event.print( outfile );
	ostr << "________________________________________"
	     << "________________________________________\n";
	ostr << "GenEvent: #" << event_number() 
	     << " ID=" << signal_process_id() 
	     << " SignalProcessGenVertex Barcode: " 
	     << ( signal_process_vertex() ? signal_process_vertex()->barcode()
		  : 0 )
	     << "\n";
	ostr << " Current Memory Usage: " 
	     << GenEvent::counter() << " events, "
	     << GenVertex::counter() << " vertices, "
	     << GenParticle::counter() << " particles.\n"; 
	ostr << " Entries this event: " << vertices_size() << " vertices, "
	     << particles_size() << " particles.\n"; 
	// Random State
	ostr << " RndmState(" << m_random_states.size() << ")=";
	for ( std::vector<double>::const_iterator rs 
		  = m_random_states.begin();
	      rs != m_random_states.end(); rs++ ) { ostr << *rs << " "; }
	ostr << "\n";
	// Weights
	ostr << " Wgts(" << weights().size() << ")=";
	for ( WeightContainer::const_iterator wgt = weights().begin();
	      wgt != weights().end(); wgt++ ) { ostr << *wgt << " "; }
	ostr << "\n";
	ostr << " EventScale " << event_scale() 
	     << " GeV \t alphaQCD=" << alphaQCD() 
	     << "\t alphaQED=" << alphaQED() << std::endl;
	// print a legend to describe the particle info
	char particle_legend[80];
	sprintf( particle_legend,"      %9s %8s %9s,%9s,%9s,%8s %4s %9s",
		 "Barcode","PDG ID","( Px","Py","Pz","E )","Stat","DecayVtx");
	ostr << "                                    GenParticle Legend\n"
	     << particle_legend << "\n";
	ostr << "________________________________________"
	     << "________________________________________\n";
	// Print all Vertices
	for ( GenEvent::vertex_const_iterator vtx = this->vertices_begin();
	      vtx != this->vertices_end(); ++vtx ) {
	    (*vtx)->print(ostr); 
	}
	ostr << "________________________________________"
	     << "________________________________________" << std::endl;
    }

    bool GenEvent::add_vertex( GenVertex* vtx ) {
	// returns true if successful - generally will only return false
	// if the inserted vertex is already included in the event.
	if ( !vtx ) return 0;
	// if vtx previously pointed to another GenEvent, remove it from that
	// GenEvent's list
	if ( vtx->parent_event() && vtx->parent_event() != this ) {
	    bool remove_status = vtx->parent_event()->remove_vertex( vtx );
	    if ( !remove_status ) {	       
		std::cerr << "GenEvent::add_vertex ERROR "
			  << "GenVertex::parent_event points to \n"
			  << "an event that does not point back to the "
			  << "GenVertex. \n This probably indicates a deeper "
			  << "problem. " << std::endl;
	    }
	}
	//
	// setting the vertex parent also inserts the vertex into this
	// event
	vtx->set_parent_event_( this );
	return ( m_vertex_barcodes.count(vtx->barcode()) ? true : false );
    }

    bool GenEvent::remove_vertex( GenVertex* vtx ) {
	// this removes vtx from the event but does NOT delete it.
	// returns True if an entry vtx existed in the table and was erased
	if ( m_signal_process_vertex == vtx ) m_signal_process_vertex = 0;
	if ( vtx->parent_event() == this ) vtx->set_parent_event_( 0 );
	return ( m_vertex_barcodes.count(vtx->barcode()) ? false : true );
    }
    
    void GenEvent::delete_all_vertices() {
	// deletes all vertices in the vertex container
	// (i.e. all vertices owned by this event)
	// The vertices are the "owners" of the particles, so as we delete
	//   the vertices, the vertex desctructors are automatically
	//   deleting their particles.
	if ( vertices_empty() ) return;
  	// delete each vertex individually (this deletes particles as well)
	while ( m_vertex_barcodes.begin() != m_vertex_barcodes.end() ) {
	    GenVertex* vtx = ( m_vertex_barcodes.begin() )->second;
            m_vertex_barcodes.erase( m_vertex_barcodes.begin() );
            delete vtx;
 	}
	//
	// Error checking:
	if ( !vertices_empty() || ! particles_empty() ) {
	    std::cerr << "GenEvent::delete_all_vertices strange result ... "
		      << "after deleting all vertices, \nthe particle and "
		      << "vertex maps aren't empty.\n  This probably " 
		      << "indicates deeper problems or memory leak in the "
		      << "code." << std::endl;
            std::cerr << "Number vtx,particle the event after deleting = "
                      << m_vertex_barcodes.size() << "  " 
		      << m_particle_barcodes.size() << std::endl;
            std::cerr << "Total Number vtx,particle in memory "
                      << "after method called = "
                      << GenVertex::counter() << "\t"
		      << GenParticle::counter() << std::endl;
	}
    }
    
    bool GenEvent::set_barcode( GenParticle* p, int suggested_barcode )
    {
	if ( p->parent_event() != this ) {
	    std::cerr << "GenEvent::set_barcode attempted, but the argument's"
		      << "\n parent_event is not this ... request rejected."
		      << std::endl;
	    return false;
	}
	//
	// First case --- a valid barcode has been suggested
	//     (valid barcodes are numbers greater than zero)
	bool insert_success = true;
	if ( suggested_barcode > 0 ) {
	    if ( m_particle_barcodes.count(suggested_barcode) ) {
		// the suggested_barcode is already used.
		if ( m_particle_barcodes[suggested_barcode] == p ) {
		    // but it was used for this particle ... so everythings ok
		    p->set_barcode_( suggested_barcode );
		    return true;
		}
		insert_success = false;
		suggested_barcode = 0;
	    } else { // suggested barcode is OK, proceed to insert
		m_particle_barcodes[suggested_barcode] = p;
		p->set_barcode_( suggested_barcode );
		return true;
	    }
	}
	//
	// Other possibility -- a valid barcode has not been suggested,
	//    so one is automatically generated
	if ( suggested_barcode < 0 ) insert_success = false;
	if ( suggested_barcode <= 0 ) {
	    if ( !m_particle_barcodes.empty() ) {
		// in this case we find the highest barcode that was used,
		// and increment it by 1
		suggested_barcode = m_particle_barcodes.rbegin()->first;
		++suggested_barcode;
	    }
	    // For the automatically assigned barcodes, the first one
	    //   we use is 10001 ... this is just because when we read 
	    //   events from HEPEVT, we will suggest their index as the
	    //   barcode, and that index has maximum value 10000.
	    //  This way we will immediately be able to recognize the hepevt
	    //   particles from the auto-assigned ones.
	    if ( suggested_barcode <= 10000 ) suggested_barcode = 10001;
	}
	// At this point we should have a valid barcode
	if ( m_particle_barcodes.count(suggested_barcode) ) {
	    std::cerr << "GenEvent::set_barcode ERROR, this should never "
		      << "happen \n report bug to matt.dobbs@cern.ch" 
		      << std::endl;
	}
	m_particle_barcodes[suggested_barcode] = p;
	p->set_barcode_( suggested_barcode );
	return insert_success;
    }

    bool GenEvent::set_barcode( GenVertex* v, int suggested_barcode )
    {
	if ( v->parent_event() != this ) {
	    std::cerr << "GenEvent::set_barcode attempted, but the argument's"
		      << "\n parent_event is not this ... request rejected."
		      << std::endl;
	    return false;
	}
	//
	// First case --- a valid barcode has been suggested
	//     (valid barcodes are numbers greater than zero)
	bool insert_success = true;
	if ( suggested_barcode < 0 ) {
	    if ( m_vertex_barcodes.count(suggested_barcode) ) {
		// the suggested_barcode is already used.
		if ( m_vertex_barcodes[suggested_barcode] == v ) {
		    // but it was used for this vertex ... so everythings ok
		    v->set_barcode_( suggested_barcode );
		    return true;
		}
		insert_success = false;
		suggested_barcode = 0;
	    } else { // suggested barcode is OK, proceed to insert
		m_vertex_barcodes[suggested_barcode] = v;
		v->set_barcode_( suggested_barcode );
		return true;
	    }
	}
	//
	// Other possibility -- a valid barcode has not been suggested,
	//    so one is automatically generated
	if ( suggested_barcode > 0 ) insert_success = false;
	if ( suggested_barcode >= 0 ) {
	    if ( !m_vertex_barcodes.empty() ) {
		// in this case we find the highest barcode that was used,
		// and increment it by 1, (vertex barcodes are negative)
		suggested_barcode = m_vertex_barcodes.rbegin()->first;
		--suggested_barcode;
	    }
	    if ( suggested_barcode >= 0 ) suggested_barcode = -1;
	}
	// At this point we should have a valid barcode
	if ( m_vertex_barcodes.count(suggested_barcode) ) {
	    std::cerr << "GenEvent::set_barcode ERROR, this should never "
		      << "happen \n report bug to matt.dobbs@cern.ch" 
		      << std::endl;
	}
	m_vertex_barcodes[suggested_barcode] = v;
	v->set_barcode_( suggested_barcode );
	return insert_success;
    }

    /////////////
    // Static  //
    /////////////
    unsigned int GenEvent::counter() { return s_counter; }
    unsigned int GenEvent::s_counter = 0; 

} // HepMC


