#include "algo1.h"

Algo1::Algo1(void) {}

Algo1::Algo1(Session& _session, SimCell& _sim_cell) {
    session = _session;
    sim_cell = _sim_cell;
    if (session.numb_passes < 1) {
        cout << "_______________________________________________________________________________" << endl;
        cout << "Error: Algo1 has been given 0 thermal average passes" << endl;
        cout << "This is probably not what you want..." << endl;
        cout << "_______________________________________________________________________________" << endl;
    }
}

double Algo1::eval_site_chem(int site) {
    double enrg = 0;
    map<string, double>::iterator rule_itr;
    for (int i = 0; i < chem_motif_groups[site].size(); i++) {
        vector<vector<int>> motif = chem_motif_groups[site][i];
        for (int j = 0; j < motif.size(); j++) {
            string rule_key = "0."; // chem ind
            rule_key += to_string(i); // clust_ind
            vector<int> group = motif[j];
            for (int k : group) {
                rule_key += "." + to_string(chem_list[k]); // sites ind
            }
            rule_itr = rule_map_chem.find(rule_key);
            enrg += (rule_itr != rule_map_chem.end()) ? (rule_itr->second / group.size()) : 0.0;
        }
    }
    return enrg;
}

double Algo1::eval_site_spin(int site) {
    double enrg = 0;
    map<string, double>::iterator rule_itr;
    for (int i = 0; i < spin_motif_groups[site].size(); i++) {
        vector<vector<int>> motif = spin_motif_groups[site][i];
        for (int j = 0; j < motif.size(); j++) {
            string rule_key = "1."; // spin ind
            rule_key += to_string(i); // clust_ind
            vector<int> group = motif[j];
            float spin_prod = 1;
            for (int k : group) {
                rule_key += "." + to_string(chem_list[k]); // sites ind
                spin_prod *= spin_list[k];
            }
            rule_itr = rule_map_spin.find(rule_key);
            enrg += (rule_itr != rule_map_spin.end()) ? (rule_itr->second * spin_prod / group.size()) : 0.0;
        }
    }
    return enrg;
}

double Algo1::eval_spin_flip(int site, float old_spin) {
    double enrg = 0;
    map<string, double>::iterator rule_itr;
    for (int i = 0; i < spin_motif_groups[site].size(); i++) {
        vector<vector<int>> motif = spin_motif_groups[site][i];
        for (int j = 0; j < motif.size(); j++) {
            string rule_key = "1.";
            rule_key += to_string(i);
            vector<int> group = motif[j];
            float spin_prod = 1;
            for (int k : group) {
                rule_key += "." + to_string(chem_list[k]);
                if (k != site) { spin_prod *= spin_list[k]; }
            }
            rule_itr = rule_map_spin.find(rule_key);
            enrg += (rule_itr != rule_map_spin.end()) ? (rule_itr->second * spin_prod) : 0.0;
        }
    }
    return (enrg * spin_list[site] - enrg * old_spin);
}

double Algo1::eval_lat() {
    double enrg = 0;
    for (int site = 0; site < sim_cell.numb_atoms; site++) {
        enrg += eval_site_chem(site);
        enrg += eval_site_spin(site);
    }
    return enrg + session.intercept * sim_cell.numb_atoms;
}

double Algo1::eval_lat_spin() {
    double enrg = 0;
    for (int site = 0; site < sim_cell.numb_atoms; site++) {
        enrg += eval_site_spin(site);
    }
    return enrg;
}

bool Algo1::bc_check(vector<float> check_vect, vector<float>& pos) {
    bool bc_test = false;
    vector<int> dir { -1, 1 };
    vector<float> bc_pos { 0, 0, 0 };
    vector<float> lc_shift { 0, 0, 0 };
    vector<vector<float>> bc_shifts { pos };

    for (int a : dir) {
        for (int i = 0; i < 3; i++) {
            lc_shift = scale_vect(sim_cell.lat_vect[i], a);
            bc_pos = vect_add(pos, lc_shift);
            bc_shifts.push_back(bc_pos);
        }
        for (int b : dir) {
            lc_shift = scale_vect(sim_cell.lat_vect[0], a);
            bc_pos = vect_add(pos, lc_shift);
            lc_shift = scale_vect(sim_cell.lat_vect[1], b);
            bc_pos = vect_add(bc_pos, lc_shift);
            bc_shifts.push_back(bc_pos);

            lc_shift = scale_vect(sim_cell.lat_vect[0], a);
            bc_pos = vect_add(pos, lc_shift);
            lc_shift = scale_vect(sim_cell.lat_vect[2], b);
            bc_pos = vect_add(bc_pos, lc_shift);
            bc_shifts.push_back(bc_pos);

            lc_shift = scale_vect(sim_cell.lat_vect[1], a);
            bc_pos = vect_add(pos, lc_shift);
            lc_shift = scale_vect(sim_cell.lat_vect[2], b);
            bc_pos = vect_add(bc_pos, lc_shift);
            bc_shifts.push_back(bc_pos);

            for (int c : dir) {
                lc_shift = scale_vect(sim_cell.lat_vect[0], a);
                bc_pos = vect_add(pos, lc_shift);
                lc_shift = scale_vect(sim_cell.lat_vect[1], b);
                bc_pos = vect_add(bc_pos, lc_shift);
                lc_shift = scale_vect(sim_cell.lat_vect[2], c);
                bc_pos = vect_add(bc_pos, lc_shift);
                bc_shifts.push_back(bc_pos);
            }
        }
    }
    for (vector<float> check : bc_shifts) {
        if (pos_comp(check, check_vect)) {
            bc_test = true;
            break;
        }
    }
    return bc_test;
}

void Algo1::fill_SMG(vector<vector<int>>& neigh_ind_list) {
    vector<float> new_pos { 0.0, 0.0, 0.0 };
    vector<float> self_site { 0.0, 0.0, 0.0 };
    vector<int> sites;
    vector<vector<int>> groups;
    vector<vector<vector<int>>> motifs;
    for (int atom = 0; atom < sim_cell.numb_atoms; atom++) {
        for (vector<vector<vector<float>>> clust : session.spin_motif_list) {
            for (vector<vector<float>> motif : clust) {
                for (vector<float> shift : motif) {
                    if (pos_comp(shift, self_site)) { sites.push_back(atom); }
                    else {
                        new_pos = vect_add(pos_list[atom], shift);
                        for (int neigh : neigh_ind_list[atom]) {
                            if (bc_check(pos_list[neigh], new_pos)) {
                                sites.push_back(neigh);
                            }
                        }
                    }
                }
                groups.push_back(sites);
                sites.clear();
            }
            motifs.push_back(groups);
            groups.clear();
        }
        spin_motif_groups.push_back(motifs);
        motifs.clear();
    }
}

void Algo1::fill_CMG(vector<vector<int>>& neigh_ind_list) {
    vector<float> new_pos { 0.0, 0.0, 0.0 };
    vector<float> self_site { 0.0, 0.0, 0.0 };
    vector<int> sites;
    vector<vector<int>> groups;
    vector<vector<vector<int>>> motifs;
    for (int atom = 0; atom < sim_cell.numb_atoms; atom++) {
        for (vector<vector<vector<float>>> clust : session.chem_motif_list) {
            for (vector<vector<float>> motif : clust) {
                for (vector<float> shift : motif) {
                    if (pos_comp(shift, self_site)) { sites.push_back(atom); }
                    else {
                        new_pos = vect_add(pos_list[atom], shift);
                        for (int neigh : neigh_ind_list[atom]) {
                            if (bc_check(pos_list[neigh], new_pos)) {
                                sites.push_back(neigh);
                            }
                        }
                    }
                }
                groups.push_back(sites);
                sites.clear();
            }
            motifs.push_back(groups);
            groups.clear();
        }
        chem_motif_groups.push_back(motifs);
        motifs.clear();
    }
}

void Algo1::print_state(string contcar_name, int temp) {
    vector<int> perm;
    vector<int> temp_spec;
    vector<float> temp_spin;
    vector<vector<float>> temp_pos;
    temp_spec.assign(chem_list.begin(), chem_list.end());
    temp_spin.assign(spin_list.begin(), spin_list.end());
    temp_pos = pos_list;// insert(temp_pos.end(), pos_list.begin(), pos_list.end());
    perm = vect_permut(temp_spin);
    sort_vect(temp_spin, perm);
    sort_vect(temp_pos, perm);
    sort_vect(temp_spec, perm);
    perm = vect_permut(temp_spec);
    sort_vect(temp_spin, perm);
    sort_vect(temp_pos, perm);
    sort_vect(temp_spec, perm);
    ofstream OUT_file;
    string file_name = contcar_name + "_" + to_string(temp);
    if (temp == -1) { file_name = contcar_name; }
    OUT_file.open(file_name);
    if (OUT_file.is_open()) {
        for (string spec : session.species_str) { OUT_file << " " << spec; }
        OUT_file << "\n 1 \n";
        for (int i = 0; i < 3; i++) {
            vector<float>vect = sim_cell.unit_lat_vect[i];
            for (int j = 0; j < 3; j++) { OUT_file << vect[j] * session.shape[i] << " "; }
            OUT_file << "\n";
        }
        for (string spec : session.species_str) { OUT_file << " " << spec; }
        OUT_file << "\n";
        for (int i = 0; i < sim_cell.species_numbs.size(); i++) { OUT_file << sim_cell.species_numbs[i] << " "; }
        OUT_file << "\nCartesian\n";
        for (int i = 0; i < sim_cell.numb_atoms; i++) {
            OUT_file << temp_pos[i][0] << " " << temp_pos[i][1] << " " << temp_pos[i][2] << " ";
            OUT_file << " # " << temp_spec[i] << " " << temp_spin[i] << "\n";
        }
    }
    OUT_file.close();
}

void Algo1::run() {
    // declare variables
    RunningStat rs_E;
    RunningStat rs_M;
    double init_spin = 0.0;
    double init_enrg = 0.0;
    bool same_spin;
    int attempts = 0;
    float rand_spin = 0.0;
    float sro_target = session.sro_target;
    float temp1 = session.start_temp;
    float temp2 = session.end_temp;
    float temp_step = session.temp_step;
    float keep_rand;
    float keep_prob;
    vector<vector<float>> spin_states = session.spin_states;
    int passes = session.ta_passes + session.eq_passes;
    int ta_passes = session.ta_passes;
    int eq_passes = session.eq_passes;
    int numb_atoms = sim_cell.numb_atoms;
    int numb_neighbors = sim_cell.atom_list[0].getNumbNeighbors(0, sim_cell);
    vector<vector<int>> neigh_ind_list(numb_atoms, vector<int>(sim_cell.atom_list[0].getNumbNeighbors(0, sim_cell), 0));
    vector<int> spin_atoms; // atomic species that can have spin

    // setup rng for random spin choice and acceptance probability
    std::mt19937_64 rng;
    uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
    rng.seed(ss);
    std::uniform_real_distribution<double> unif(0, 1);
    
    cout << "Making atom list and neighbor index list\n";
    // Make atom_list more acessable for species and spin and neighbors
    for (int i = 0; i < sim_cell.numb_atoms; i++) {
        chem_list.push_back(sim_cell.atom_list[i].getSpecies());
        spin_list.push_back(sim_cell.atom_list[i].getSpin());
        pos_list.push_back({ sim_cell.atom_list[i].pos[0], sim_cell.atom_list[i].pos[1], sim_cell.atom_list[i].pos[2] });
        for (int j = 0; j < numb_neighbors; j++) {
            neigh_ind_list[i][j] = sim_cell.atom_list[i].getNeighborIndex(j, sim_cell);
        }
    }
    
    cout << "Making rule maps\n";
    // Make rule_maps for easy lookup
    string rule_key;
//    for (Rule rule : session.chem_rule_list) {
//        rule_key = to_string(rule.GetType()) + "." + to_string(rule.clust_ind);
//        for (int i = 0; i < rule.deco.size(); i++) { rule_key += "." + to_string(rule.deco[i]); }
//        rule_map_chem.insert(pair<string, double>(rule_key, rule.GetEnrgCont()));
//    }
    for (Rule rule : session.spin_rule_list) {
        rule_key = to_string(rule.GetType()) + "." + to_string(rule.clust_ind);
        for (int i = 0; i < rule.deco.size(); i++) { rule_key += "." + to_string(rule.deco[i]); }
        rule_map_spin.insert(pair<string, double>(rule_key, rule.GetEnrgCont()));
        for (int atom : rule.deco) {
            if (find(spin_atoms.begin(), spin_atoms.end(), atom) == spin_atoms.end()) { spin_atoms.push_back(atom); } // initialize spin_atoms
        }
    }
    
    // Fill motif group lists
//    cout << "Making chem motif group lists\n";
//    fill_CMG(neigh_ind_list);
    cout << "Making spin motif group lists\n";
    fill_SMG(neigh_ind_list);
    
    // Create seperate output file to avoid race condition
    string file_name = "OUTPUT";
    string contcar_name = "CONTCAR";
    bool file_exists = true;
    while (file_exists == true) {
        const char* c_file = file_name.c_str();
        int fd = open(c_file, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            // file exists or otherwise uncreatable
            outfile_count += 1;
            file_name = "OUTPUT" + to_string(outfile_count);
            contcar_name = "CONTCAR" + to_string(outfile_count);
        }
        else {
            file_exists = false;
            close(fd);
        }
    }
    const char* c_file = file_name.c_str();
    ofstream Output;
    Output.open(c_file);
    // Output energy and spin for convergence test
    ofstream Output_converge;
    if ( session.do_conv_output) { Output_converge.open("OUTPUT_CONVERG"); }

    // Begin MC
    init_enrg = eval_lat_spin();
    cout << "Initial spin energy is " << init_enrg / numb_atoms << " per atom\n";
    for (int site = 0; site < numb_atoms; site++) {
        if (find(spin_atoms.begin(), spin_atoms.end(), chem_list[site]) != spin_atoms.end()) {
            init_spin += spin_list[site];
        }
    }
    cout << "Initial spin is " << init_spin / numb_atoms << " per atom\n";
    Output << "Using Algo1 for spin flip";
    Output << "\nPhase: " << sim_cell.phase_init;
    Output << "\nComposition: ";
    for (int i = 0; i < sim_cell.species_numbs.size(); i++) { Output << sim_cell.species_numbs[i] << " "; }
    Output << "\nMC passes: " << passes;
    Output << "\nInitial spin energy per atom: ";
    Output << init_enrg / numb_atoms;
    Output << "\nInitial spin per atom: ";
    Output << init_spin / numb_atoms;
    Output << "\ntemp, enrg, mag, var_e, var_spin, Cmag, Xmag, flip_count, flip_count2" << endl;
    
    //    // initalize system with desired SRO
    //    Output << "EQ passes: " << session.eq_passes << ", EQ Temp: " << session.sro_temp << "\n";
    //    Output << "SRO Target: " << session.sro_target << "\n";
    //    cout << "SRO Target: " << session.sro_target << "\n";
    //    cout << "Starting Real MC\n";
    
    // Start MC loop
    cout << "Entering main loop\n";
    int temp_count = 0; // index of temperature
    float inc_dir = 1; // temp increment direction
    if (signbit(temp2 - temp1) == 1) { inc_dir = -1; }
    for (float temp = temp1; (temp2 - temp) * inc_dir >= 0; temp += temp_step * inc_dir) {
        int flip_count = 0;
        int flip_count2 = 0;
        for (int pass = 0; pass < passes; pass++) {
            for (int site = 0; site < numb_atoms; site++) {
                double e_flip = 0.0;
                float spin_flip = 0.0;
                float old_spin = spin_list[site];
                float new_spin = 0.0;
                int state = 1;
                while (state != DONE) {
                    switch (state) {
                    case 0:
                        state = DONE;
                        break;
                    case 1:
                        if (find(spin_atoms.begin(), spin_atoms.end(), chem_list[site]) == spin_atoms.end()) {
                            state = 0; //NO_SPIN
                            break;
                        }// state set to 0
                        if (spin_states[chem_list[site]].size() <= 1) {
                            state = 0; //ONE_SPIN
                            break;
                        }
                        // Flip Spin
                        attempts = 0;
                        same_spin = true;
                        while (same_spin == true and attempts < 20) {
                            rand_spin = unif(rng);
                            for (int it_spin_state = 0; it_spin_state < spin_states[chem_list[site]].size(); it_spin_state++) {
                                if (rand_spin > float(it_spin_state) / float(spin_states[chem_list[site]].size())) {
                                    new_spin = spin_states[chem_list[site]][it_spin_state]; }
                            }
                            if (new_spin != old_spin) { same_spin = false; }
                            attempts += 1;
                        }
                        if (attempts >= 20) {
                            state = 0;
                            break;
                        }
                        spin_list[site] = new_spin;
                        e_flip = eval_spin_flip(site, old_spin);
                        spin_flip = new_spin - old_spin;
                        if (e_flip < 0) { flip_count += 1; }
                        else {
                            keep_rand = unif(rng);
                            keep_prob = exp(-1 / (Kb * temp) * (e_flip));
                            if (keep_rand < keep_prob) { flip_count2 += 1; }
                            else {spin_list[site] = old_spin; e_flip = 0.0; spin_flip = 0.0; }
                        }
                        // Record the enrg and spin changes
                        init_enrg += e_flip;
                        init_spin += spin_flip;
                        state = DONE;
                        break;
                        }
                    }
                if (pass >= eq_passes) {
                    rs_E.Push(init_enrg);
                    rs_M.Push(init_spin);
                }
            }
            if (session.do_conv_output ) {
                Output_converge << init_enrg << " " << init_spin << endl;
            }
        }
        double e_avg = rs_E.Mean() / numb_atoms;
        double spin_avg = rs_M.Mean() / numb_atoms;
        double var_e = rs_E.Variance();
        double var_spin = rs_M.Variance();
        double Cmag = var_e / (Kb * pow(temp, 2));
        double Xmag = var_spin / (Kb * pow(temp, 2));
        Output << temp << ", "
            << e_avg << ", "
            << spin_avg << ", "
            << var_e << ", "
            << var_spin << ", "
            << Cmag << ", "
            << Xmag << ", "
            << flip_count << ", "
            << flip_count2 << endl;
        rs_E.Clear();
        rs_M.Clear();
        if (session.write_contcars == true) {
            print_state(contcar_name, temp_count);
        }
        temp_count += 1;
    }
    print_state("CONTCAR_FINAL", -1);
    Output.close();
    cout << " MC Finished\n";
}
