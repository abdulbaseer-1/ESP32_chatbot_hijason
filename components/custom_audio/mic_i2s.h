#ifndef MIC_I2S_H
#define MIC_I2S_H

void i2s_mic_init(void);
int i2s_mic_read(char* buffer, int len);

#endif
