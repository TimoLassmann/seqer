#include "emit_random.h"

static int emit_a_kmer(struct fhmm* fhmm,  struct ihmm_sequence* s,int state, int len,  rk_state* rndstate);
static int emit_a_sequence(struct fhmm* fhmm,  struct ihmm_sequence* s, rk_state* rndstate );

struct seq_buffer* emit_kmers_from_state(struct fhmm* fhmm,int start_state, int num,int len,rk_state* rndstate)
{
        struct seq_buffer* sb_out = NULL;
        int i;

        ASSERT(fhmm != NULL, "No HMM");
        ASSERT(num != 0, "No sequences requested.");


        MMALLOC(sb_out,sizeof(struct seq_buffer));
        sb_out->malloc_num = num ;
        sb_out->num_seq = num;
        sb_out->sequences = NULL;
        sb_out->max_len = 0.0;
        sb_out->L = fhmm->L;

        //sb_out->background = NULL;
        //MMALLOC(sb_out->background,sizeof(double) * sb_out->L );

        MMALLOC(sb_out->sequences, sizeof(struct ihmm_sequence*) *sb_out->malloc_num );

         for(i = 0; i < sb_out->num_seq;i++){
                sb_out->sequences[i] = NULL;
                RUNP(sb_out->sequences[i] = alloc_ihmm_seq());
                snprintf(sb_out->sequences[i]->name, 256, "RANDOM%d", i+1);
                while(sb_out->sequences[i]->malloc_len <= len){ /* make sure we have enough memory to store sequence of len "len" */
                        realloc_ihmm_seq(sb_out->sequences[i]);
                }

                RUN(emit_a_kmer(fhmm, sb_out->sequences[i], start_state,len, rndstate));
        }

        return sb_out;
ERROR:
        return NULL;
}

struct seq_buffer* emit_sequences_from_fhmm_model(struct fhmm* fhmm, int num, rk_state* rndstate)
{
        struct seq_buffer* sb_out = NULL;
        int i;
        double s1,s2;

        ASSERT(fhmm != NULL, "No HMM");
        ASSERT(num != 0, "No sequences requested.");



        /*double test[4];


        for(i =0; i < 1000;i++){
                sum = 0.0;
                for(j = 0; j < 4;j++){
                        test[j] = rk_gamma(&rndstate, (double)(i+1) / 100.0, 1.0);
                        sum += test[j];
                }
                for(j = 0; j < 4;j++){
                        test[j] /= sum;
                        fprintf(stdout,"%f\t",test[j]);
                }
                fprintf(stdout,"%f\n", (double)(i+1) / 100.0);

        }
        exit(1);*/





        MMALLOC(sb_out,sizeof(struct seq_buffer));
        sb_out->malloc_num = num ;
        sb_out->num_seq = num;
        sb_out->sequences = NULL;
        sb_out->max_len = 0.0;
        sb_out->L = fhmm->L;

        //sb_out->background = NULL;
        //MMALLOC(sb_out->background,sizeof(double) * sb_out->L );

        MMALLOC(sb_out->sequences, sizeof(struct ihmm_sequence*) *sb_out->malloc_num );

        s1 = 0.0;
        s2 = 0.0;
        for(i = 0; i < sb_out->num_seq;i++){
                sb_out->sequences[i] = NULL;
                RUNP(sb_out->sequences[i] = alloc_ihmm_seq());
                snprintf(sb_out->sequences[i]->name, 256, "RANDOM%d", i+1);
                while (sb_out->sequences[i]->seq_len < 16){
                        RUN(emit_a_sequence(fhmm, sb_out->sequences[i],rndstate));
                }



                s1 += sb_out->sequences[i]->seq_len;
                s2 += (sb_out->sequences[i]->seq_len * sb_out->sequences[i]->seq_len);
        }

        s2 = sqrt(((double) sb_out->num_seq * s2 - s1 * s1)/ ((double) sb_out->num_seq * ((double) sb_out->num_seq -1.0)));
        s1 = s1 / (double) sb_out->num_seq;
        LOG_MSG("Simulated %d sequences of mean length %f stdev %f", sb_out->num_seq,s1,s2);
        return sb_out;
ERROR:
        return NULL;
}

int emit_a_sequence(struct fhmm* fhmm,  struct ihmm_sequence* s, rk_state* rndstate)
{
        int state = START_STATE;
        int j;
        double r;
        while(state != END_STATE){
                /* transistion */

                r = rk_double(rndstate);

                for(j = 0; j < fhmm->K;j++){
                        if(r <= fhmm->t[state][j]){
                                state = j;
                                break;
                        }
                }

                /* emission */
                r = rk_double(rndstate);
                for(j = 0; j < fhmm->L;j++){

                        if(r <= fhmm->e[state][j]){
                                s->seq[s->seq_len] = j;
                                s->seq_len++;


                                break;

                        }
                }
                if(s->seq_len == s->malloc_len){
                        s->malloc_len = s->malloc_len << 1;
                        RUN(realloc_ihmm_seq(s));
                }
        }

        return OK;
ERROR:
        return FAIL;
}

int emit_a_kmer(struct fhmm* fhmm,  struct ihmm_sequence* s,int state, int len,  rk_state* rndstate)
{
        int j;
        double r;

        while(state != END_STATE &&  s->seq_len != len ){

                /* emission */
                r = rk_double(rndstate);
                for(j = 0; j < fhmm->L;j++){

                        if(r <= fhmm->e[state][j]){
                                s->seq[s->seq_len] = j;
                                s->seq_len++;


                                break;

                        }
                }
                if(s->seq_len == s->malloc_len){
                        s->malloc_len = s->malloc_len << 1;
                        RUN(realloc_ihmm_seq(s));
                }
                /* transistion */

                r = rk_double(rndstate);

                for(j = 0; j < fhmm->K;j++){
                        if(r <= fhmm->t[state][j]){
                                state = j;
                                break;
                        }
                }


        }
        // LOG_MSG("requested: %d emitted:%d",len, s->seq_len);

        return OK;
ERROR:
        return FAIL;
}


struct seq_buffer* emit_sequences_from_random_model(struct seq_buffer* sb_in, int num, rk_state* rndstate)
{
        struct seq_buffer* sb_out = NULL;

        int i,j,c;
        double s1,s2;
        //double s1_t,s2_t;
        double r;



        ASSERT(sb_in != NULL, "No sequence Buffer");
        s1 = 0.0;
        s2 = 0.0;
        for(i = 0; i < sb_in->num_seq;i++){
                //sb_in->sequences[i]->seq_len = 10 + (int)(rk_double(&rndstate)*10.0) - 5.0;
                s1 += sb_in->sequences[i]->seq_len;
                s2 += (sb_in->sequences[i]->seq_len * sb_in->sequences[i]->seq_len);
        }

        s2 = sqrt(((double) sb_in->num_seq * s2 - s1 * s1)/ ((double) sb_in->num_seq * ((double) sb_in->num_seq -1.0)));
        s1 = s1 / (double) sb_in->num_seq;



        LOG_MSG("Mean length: %f stdev: %f L:%d",s1,s2, sb_in->L);

        /*for(j = 0; j < 10;j++){
                s1_t = 0.0;
                s2_t = 0.0;
                for(i = 0;i < sb_in->num_seq;i++){
                        r = rk_normal(&rndstate, s1,s2);
                        s1_t += r;
                        s2_t += r*r;
                        //fprintf(stdout,"%f ", r);
                }
                s2_t = sqrt(((double) sb_in->num_seq * s2_t - s1_t * s1_t)/ ((double) sb_in->num_seq * ((double) sb_in->num_seq -1.0)));
                s1_t = s1_t / (double) sb_in->num_seq;

                LOG_MSG("Mean length: %f stdev: %f",s1_t,s2_t);
        }
        */

        MMALLOC(sb_out,sizeof(struct seq_buffer));
        sb_out->malloc_num = num ;
        sb_out->num_seq = num;
        sb_out->sequences = NULL;
        sb_out->max_len = 0.0;
        sb_out->L = sb_in->L;

        //sb_out->background = NULL;
        //MMALLOC(sb_out->background,sizeof(double) * sb_out->L );
        for(i = 0;i < sb_out->L;i++){
                //        sb_out->background[i] = sb_in->background[i];
                //fprintf(stdout,"%d %f",i,sb_out->background[i]);
        }


        for(i = 1;i < sb_out->L;i++){
                //sb_out->background[i] += sb_out->background[i-1];
        }


        MMALLOC(sb_out->sequences, sizeof(struct ihmm_sequence*) *sb_out->malloc_num );
        for(i = 0; i < sb_out->num_seq;i++){
                sb_out->sequences[i] = NULL;
                RUNP(sb_out->sequences[i] = alloc_ihmm_seq());


                snprintf(sb_out->sequences[i]->name, 256, "RANDOM%d", i+1);
                sb_out->sequences[i]->seq_len  = 0;

                while(sb_out->sequences[i]->seq_len  < 16){
                        sb_out->sequences[i]->seq_len = rk_normal(rndstate, s1,s2);
                }


                if( sb_out->sequences[i]->seq_len > sb_out->max_len){
                        sb_out->max_len = sb_out->sequences[i]->seq_len;
                }
                while(sb_out->sequences[i]->malloc_len <= sb_out->sequences[i]->seq_len){
                        RUN(realloc_ihmm_seq(sb_out->sequences[i]));
                }
                if(sb_out->sequences[i]->malloc_len ==  sb_out->sequences[i]->seq_len){
                        fprintf(stdout," %d %d \n",sb_out->sequences[i]->malloc_len, sb_out->sequences[i]->seq_len );
                }

                for(j = 0;j < sb_out->sequences[i]->seq_len;j++){
                        //r = random_float_zero_to_x(1.0);
                         r = rk_double(rndstate);

                         //fprintf(stdout,"%f\n",r);
                         for(c = 0 ; c < sb_out->L;c++){

                                 //if(r <= sb_out->background[c]){
                                 //sb_out->sequences[i]->seq[j] = c;
                                 //break;
                                 //}
                         }
                }
        }

        return sb_out;
ERROR:
        return NULL;
}
