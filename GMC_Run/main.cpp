#include <cstdio>
#include <iostream>
#include <string>
#include "run_sim.h"
#include "session.h"
#include "utils.h"
using namespace std;
// The MC code
// The paramiters for the MC run ( Composition, Simulation cell size, ect...) are all set in INPUT.txt
// These settings are read in and stored in the SimCell object. The SimCell object is then reffrenced to set up the actual MC run.
SimCell sim_cell; // The simulation cell object. This object contains all the information about the simulation cell and the atoms in it.
int main(int argc, char** argv) {
    string input = "INPUT";
    string poscar = "POSCAR";
    string spin_states = "SPIN_STATES";
    string cluster_file = "CLUSTERS";
    vector<string> setting;
    for (int i = 1; i < argc; i++) {
        setting = split(argv[i], "=");
        if (setting[0].compare("-I") == 0) { input = setting[1]; }
        if (setting[0].compare("-P") == 0) { poscar = setting[1]; }
        if (setting[0].compare("-S") == 0) { spin_states = setting[1]; }
        if (setting[0].compare("-C") == 0) { cluster_file = setting[1]; }
    }
	Session ses(input); // settings for MC run from input file
	if (ses.use_states == true) { ses.add_spin_states(spin_states); }
	ses.fill_rule_list(cluster_file); // Initalize the "cluster rules" that will be used to properly apply the ECI to each site in the simulation cell.
	ses.find_unique_dists(); // Find the list of distances used in mc_rules
	sim_cell.initSimCell(poscar, ses); // Create the simulation cell object. Arguments (POSCAR_file, dist_list, shape, species numbs, cutoff (currently unused), sim_type (also unused), phase_init (aust/mart), spin_init (AFM/FM/RAND), species_init (Ordered/Random), bool use_poscar)
	Sim sim(ses, sim_cell);
	sim.start();
	return 0;
}
