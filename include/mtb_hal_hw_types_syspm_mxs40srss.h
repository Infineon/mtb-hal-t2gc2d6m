/***************************************************************************//**
* \file mtb_hal_hw_types_syspm_mxs40srss.h
*
*********************************************************************************
* \copyright
* Copyright(c) 2024-2026 Infineon Technologies AG or an affiliate of
* Infineon Technologies AG
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#pragma once

#include "cy_pdl.h"
#include "mtb_hal_hw_types_syspm_srss.h"

#if defined(CY_IP_MXS40SRSS)

// The following values do not apply to this IP, give them
// placeholder values to satisfy the interface
#define MTB_HAL_MAP_SYSPM_AFTER_DS_WFI_TRANSITION  (5u)
#define MTB_HAL_MAP_SYSPM_CB_DEEPSLEEP_RAM         (4u)
#define MTB_HAL_MAP_SYSPM_CB_NORMAL                (5u)
#define MTB_HAL_MAP_SYSPM_CB_LOW                   (6u)
#define MTB_HAL_MAP_SYSPM_CB_HIGH                  (7u)
#define MTB_HAL_MAP_SYSPM_CB_HIBERNATE_RAM         (9u)

#endif // if defined(CY_IP_MXS40SRSS)
