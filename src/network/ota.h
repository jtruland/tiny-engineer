#pragma once

#ifdef TE_OTA
void startOta();
void pollOta();
#else
inline void startOta() {}
inline void pollOta() {}
#endif
