
#include "run_score.h"

#include "sequence_struct.h"

#include "tllogsum.h"

int run_score_sequences(struct fhmm* fhmm, struct seq_buffer* sb,struct seqer_thread_data** td)
{

        int i;
        int num_threads;
        ASSERT(fhmm != NULL,"no model");
        ASSERT(sb != NULL, "no parameters");

        /* just to be 100% safe... */
        init_logsum();


        /* allocate dyn programming matrices.  */
        RUN(realloc_dyn_matrices(fhmm, sb->max_len+1));
        //LOG_MSG("new len: %d states:%d", sb->max_len,fhmm->K);
        num_threads = td[0]->num_threads;

        /* score sequences  */
        /*
        for(i = 0; i <  td[0]->num_threads;i++){
                td[i]->sb = sb;
                td[i]->fhmm = fhmm;
                if(thr_pool_queue(pool, do_score_sequences,td[i]) == -1){
                        fprintf(stderr,"Adding job to queue failed.");
                }
        }
        thr_pool_wait(pool);
        */
        for(i = 0; i < num_threads;i++){
                td[i]->sb = sb;
                td[i]->fhmm = fhmm;
        }


#ifdef HAVE_OPENMP
        omp_set_num_threads(num_threads);
#pragma omp parallel shared(td) private(i)
        {
#pragma omp for schedule(dynamic) nowait
#endif
                for(i = 0; i < num_threads;i++){
                        do_score_sequences(td[i]);
                }
#ifdef HAVE_OPENMP
        }
#endif

        return OK;
ERROR:

        return FAIL;
}

int run_label_sequences(struct fhmm* fhmm, struct seq_buffer* sb, int num_threads)
{

        //struct thr_pool* pool = NULL;
        struct seqer_thread_data** td = NULL;
        int i;
        ASSERT(fhmm != NULL,"no model");
        ASSERT(sb != NULL, "no parameters");

        init_logsum();
        /* start threadpool  */
        //if((pool = thr_pool_create(num_threads ,num_threads, 0, 0)) == NULL) ERROR_MSG("Creating pool thread failed.");


        /* allocate data for threads; */
        RUNP(td = create_seqer_thread_data(&num_threads,(sb->max_len+2)  , fhmm->K ,NULL, THREAD_DATA_FULL));// & sb->rndstate));

        /* score sequences  */

        /*for(i = 0; i < num_threads;i++){
                td[i]->sb = sb;
                td[i]->fhmm = fhmm;
                if(thr_pool_queue(pool, do_label_sequences,td[i]) == -1){
                        fprintf(stderr,"Adding job to queue failed.");
                }
        }
        thr_pool_wait(pool);
        */
        for(i = 0; i < num_threads;i++){
                td[i]->sb = sb;
                td[i]->fhmm = fhmm;
        }


#ifdef HAVE_OPENMP
        omp_set_num_threads(num_threads);
#pragma omp parallel shared(td) private(i)
        {
#pragma omp for schedule(dynamic) nowait
#endif
                for(i = 0; i < num_threads;i++){
                        do_label_sequences(td[i]);
                }
#ifdef HAVE_OPENMP
        }
#endif


        free_seqer_thread_data(td);
        //thr_pool_destroy(pool);

        return OK;
ERROR:
        free_seqer_thread_data(td);
        //thr_pool_destroy(pool);
        return FAIL;
}



void* do_score_sequences(void* threadarg)
{
        struct seqer_thread_data *data;
        struct fhmm* fhmm = NULL;
        struct ihmm_sequence* seq = NULL;
        int i;
        int num_threads;
        int thread_id;
        int expected_len;
        double f_score;
        double r_score;
        data = (struct seqer_thread_data *) threadarg;

        num_threads = data->num_threads;
        thread_id = data->thread_ID;
        fhmm = data->fhmm;

        expected_len = 0;
        for(i = 0; i < data->sb->num_seq;i++){
                expected_len += data->sb->sequences[i]->seq_len;
        }
        expected_len = expected_len / data->sb->num_seq;
        //LOG_MSG("Average sequence length: %d",expected_len);

        for(i =0; i < data->sb->num_seq;i++){
                if( i% num_threads == thread_id){
                        seq = data->sb->sequences[i];
                        LOG_MSG("Searching %s len: %d", seq->name,seq->seq_len);
                        RUN(forward(fhmm, data->F_matrix, &f_score, seq->seq, seq->seq_len ));
                        RUN(random_model_score(seq->seq_len, &r_score));//, seq->seq, seq->seq_len,seq->seq_len));
                        fprintf(stdout,"seq:%d %f %f log-odds: %f  p:%f\n",i, f_score,r_score,f_score - r_score, LOGISTIC_FLT(f_score - r_score));

                        seq->score = (f_score - r_score) / logf(2.0);
                        //seq->score = f_score;
                }
        }
        return NULL;
ERROR:
        return NULL;
}

void* do_score_sequences_per_model(void* threadarg)
{
        struct seqer_thread_data *data;
        struct fhmm* fhmm = NULL;
        struct ihmm_sequence* seq = NULL;
        int i;
        //int num_threads;
        int thread_id;          /* this is actually the model number */

        double f_score;
        //double r_score;
        data = (struct seqer_thread_data *) threadarg;

        //num_threads = data->num_threads;
        thread_id = data->thread_ID;
        fhmm = data->fhmm;

        //LOG_MSG("Average sequence length: %d",expected_len);

        for(i =0; i < data->sb->num_seq;i++){
                        seq = data->sb->sequences[i];
                        RUN(forward(fhmm, data->dyn, &f_score, seq->seq, seq->seq_len ));
                        seq->score_arr[thread_id] = f_score;
        }
        return NULL;
ERROR:
        return NULL;
}
void* do_label_sequences(void* threadarg)
{
        struct seqer_thread_data *data;
        struct fhmm* fhmm = NULL;
        struct ihmm_sequence* seq = NULL;
        int i;
        int num_threads;
        int thread_id;
        double f_score;
        double b_score;
        data = (struct seqer_thread_data *) threadarg;

        num_threads = data->num_threads;
        thread_id = data->thread_ID;
        fhmm = data->fhmm;
        //LOG_MSG("Average sequence length: %d",expected_len);

        for(i =0; i < data->sb->num_seq;i++){
                if( i% num_threads == thread_id){
                        seq = data->sb->sequences[i];
                        RUN( forward(fhmm, data->F_matrix, &f_score, seq->seq, seq->seq_len));
                        RUN(backward(fhmm, data->B_matrix, &b_score, seq->seq, seq->seq_len));
                        RUN(posterior_decoding(fhmm,data->F_matrix,data->B_matrix,f_score,seq->seq, seq->seq_len, seq->label));
                }
        }
        return NULL;
ERROR:
        return NULL;
}
