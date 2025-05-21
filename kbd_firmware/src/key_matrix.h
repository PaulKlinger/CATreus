
 #ifdef __cplusplus
 extern "C" {
 #endif

 #include <stdint.h>
 #include <stdbool.h>
 
 struct key_coord {
        uint8_t row;
        uint8_t col;
 };

 struct pressed_keys {
    struct key_coord keys[5];
    uint8_t n_pressed;
 };

struct pressed_keys read_key_matrix(void);

void init_key_matrix(void);

bool wake_pressed(void);

int wait_for_key(int timeout_ms);

 
 #ifdef __cplusplus
 }
 #endif