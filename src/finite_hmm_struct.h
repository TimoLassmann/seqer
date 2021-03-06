#ifndef FINITE_HMM_STRUCT_H
#define FINITE_HMM_STRUCT_H

#define N_STATE 0
#define B_STATE 1
#define E_STATE 2
#define C_STATE 3
#define J_STATE 4


struct fhmm{
        float** F_matrix;
        float** B_matrix;
        float** F_NBECJ;
        float** B_NBECJ;
        float** e;
        float** t;
        int** tindex;
        float* m_comp_back; /* Equivalent (hopefully to compo in HMMER - see Biased composition filter.) */
        float* background;
        float f_score;
        float b_score;
        float r_score;
        double H;
        double lambda;
        double tau;
        int alloc_matrix_len;
        int alloc_K;
        int K;
        int L;
};

struct fhmm_dyn_mat{
        float** F_matrix;
        float** B_matrix;

        float** F_NBECJ;
        float** B_NBECJ;

        int* path;
        int alloc_matrix_len;
        int alloc_K;
};

#endif
