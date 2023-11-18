#ifndef MODBUS_H
#define MODBUS_H
uint32_t bswap_32(uint32_t x);
uint16_t bswap_16(uint16_t x);

uint16_t crc16(uint8_t *crbuffer, uint16_t crbuffer_length);
float modbus_get_float(const uint16_t u16A,const uint16_t u16B);
float modbus_get_float_abcd(const uint16_t u16A,const uint16_t u16B);
float modbus_get_float_dcba(const uint16_t u16A,const uint16_t u16B);
float modbus_get_float_badc(const uint16_t u16A,const uint16_t u16B);
float modbus_get_float_cdab(const uint16_t u16A,const uint16_t u16B);
int16_t modbus_get_16hextoint(const uint16_t hex16b);
uint16_t modbus_get_unsign_16hextoint(uint16_t hex16b);
#endif

