#ifndef __CHANNEL_INIT_HPP__
#define __CHANNEL_INIT_HPP__

#include <cstdint>

int Channel_init(int channelID, uint32_t width, uint32_t height);
int Channel_close(int channel);

#endif // __CHANNEL_INIT_HPP__
