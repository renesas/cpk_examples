/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************//**********************************************************************************************************************
 * File Name    : currency_table.h
 * Version      : .
 * Description  : .
 *********************************************************************************************************************/

/*
 * currency_table.h
 *
 *  Created on: 19 Jun 2023
 *      Author: b3800117
 */

#include <math.h>

#ifndef CURRENCY_TABLE_H
#define CURRENCY_TABLE_H

typedef struct st_currency_rates {
    double austrailian_dollar;      // AUS
    double british_pound_sterlin;   // GBP
    double canadian_dollar;         // CAN
    double chinese_yen;             // CNY
    double european_union_euro;     // EUR
    double hong_kong_dollar;        // HKD
    double indian_rupee;            // INR
    double jananese_yen;            // JPY
    double singaphorean_dollar;     // SGD
    double unitedstates_dollar;        // USD
} st_currency_rates_t;

typedef enum {
    CURRENCY_USD_AND_GBP,
    CURRENCY_USD_AND_CAD,
    CURRENCY_USD_AND_CNY,
    CURRENCY_USD_AND_EUR,
    CURRENCY_USD_AND_HKD,
    CURRENCY_USD_AND_INR,
    CURRENCY_USD_AND_JPY,
    CURRENCY_USD_AND_SGD,
    CURRENCY_USD_AND_NONE
} currency_demo_enum;


extern st_currency_rates_t aus; // Austrailian Dollar

#endif /* CURRENCY_TABLE_H */
