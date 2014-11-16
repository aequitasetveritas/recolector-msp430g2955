/*
 * nrf.h
 *
 *  Created on: Nov 1, 2014
 *      Author: escualo
 */

#ifndef NRF_H_
#define NRF_H_

void nrf_inicializar(void);
void nrf_limpiar_flags();
void nrf_conf_inicial(void);
void nrf_ver_registros(void);
void leer_rx_payload(uint8_t * buff);
void nrf_como_PTX(uint64_t dir);
void nrf_cargar_y_transmitir(uint8_t * data32bytes);
void nrf_como_PRX(uint64_t dir);
inline uint8_t nrf_esperar(void);
inline int8_t nrf_RX_pendientes(void);
inline void nrf_activar_RX(void);
#endif /* NRF_H_ */
