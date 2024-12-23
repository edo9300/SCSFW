// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Adrian "asie" Siekierka

    .thumb
    .syntax unified

    .global xor_constant
    .section .text.xor_constant, "ax"
xor_constant:
    eors r0, r1
    bx lr
