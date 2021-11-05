#pragma once


int fbd_check_fishbite(void);
void fbd_update(double val);
void fbd_sensitivity_low(void);
void fbd_sensitivity_mid(void);
void fbd_sensitivity_high(void);

void fbd_sensitivity(double value);

void fbd_set_detect_handler(void (*fn_ptr));
void fbd_set_release_handler(void (*fn_ptr));

void fbd_init(void);
