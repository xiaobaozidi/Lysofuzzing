
#ifdef __cplusplus
extern "C"{ // use c compiler 
#endif 

u32 get_shortest_fun_distance(u8 *bitmap, u32 len, u32 target);
u32 get_total_bb_distance(u8 *bitmap, u32 len, u32 target);
u32 get_shortest_bb_distance(u8 *bitmap, u32 len, u32 target);
void  init_func_distance(char *root_path);
void  init_bb_distance(char *root_path);
u32 init_bb2bb_map(char *root_path);
#ifdef __cplusplus
}
#endif
