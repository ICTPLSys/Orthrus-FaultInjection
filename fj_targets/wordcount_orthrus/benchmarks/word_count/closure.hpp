scee::mut_array<result_t> create_result_array(size_t n);

void destroy_result_array(scee::mut_array<result_t> results);

void word_count_map_worker(std::string_view input,
                           scee::mut_array<result_t> results, size_t mapper_idx,
                           map_reduce_config config);

void word_count_reduce_worker(scee::mut_array<result_t> map_results,
                              scee::mut_array<result_t> reduce_results,
                              size_t reducer_idx, map_reduce_config config);

result_t sort_results(scee::mut_array<result_t> reduce_results,
                      map_reduce_config config);

void calculate_word_frequency(scee::imm_array<kv_pair> final_results,
                              size_t final_result_size);

double calc_each_word_frequency(scee::imm_array<kv_pair> final_results, size_t final_result_size);

double calc_average_length(scee::imm_array<kv_pair> final_results, size_t final_result_size);

double calc_average_frequency(scee::imm_array<kv_pair> final_results, size_t final_result_size);

double calc_standard_deviation(scee::imm_array<kv_pair> final_results, size_t final_result_size);