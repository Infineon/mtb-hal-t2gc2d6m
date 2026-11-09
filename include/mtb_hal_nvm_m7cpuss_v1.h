/***************************************************************************//**
* File Name: mtb_hal_nvm_m7cpuss_v1.h
*
* Description:
* Provides a high level interface for interacting with the Infineon embedded
* non-volatile memory (NVM). This is wrapper around the lower level PDL API.
*
********************************************************************************
   * \copyright
* Copyright(c) 2018-2026 Infineon Technologies AG or an affiliate of
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

#include "mtb_hal_hw_types.h"
#include "mtb_hal_nvm.h"
#include "cy_sysfault.h"
#include "cy_utils.h"
#include "mtb_hal_utils_impl.h"
#include <string.h>
#include <assert.h>


#if (MTB_HAL_DRIVER_AVAILABLE_NVM)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*       Functions
*******************************************************************************/


//--------------------------------------------------------------------------------------------------
// _mtb_hal_flash_convert_status
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_flash_convert_status(uint32_t pdl_status)
{
    return (pdl_status == CY_FLASH_DRV_OPERATION_STARTED) ? CY_RSLT_SUCCESS : pdl_status;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_flash_is_sram_address
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE bool _mtb_hal_flash_is_sram_address(uint32_t address)
{
    return ((CY_SRAM_BASE <= address) && (address < (CY_SRAM_BASE + CY_SRAM_SIZE)));
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_flash_clear_cache
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE void _mtb_hal_flash_clear_cache(bool clearCache)
{
    /* This is for cache on CPU SUBSYSTEM. */
    #if defined(CY_FLASH_CBUS_BASE) && defined(CY_IP_M33SYSCPUSS)
    if (clearCache)
    {
        /* TODO: We will replace it once api is provided by PDL(DRIVERS-18245).
           Because we read flash memory using CBUS. So we have to invalidate icache after writing
           flash memory */
        ICACHE0->CMD = ICACHE0->CMD | ICACHE_CMD_INV_Msk;
    }
    #else
    CY_UNUSED_PARAMETER(clearCache);
    #endif
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_flash_run_operation
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_flash_run_operation(
    _mtb_hal_flash_operation operation, uint32_t address, const uint32_t* data, bool clearCache,
    bool blocking)
{
    cy_rslt_t status = MTB_HAL_NVM_RSLT_ERR_ADDRESS;

    if (_mtb_hal_flash_is_sram_address((uint32_t)data))
    {
        status = (cy_rslt_t)_mtb_hal_flash_convert_status((cy_rslt_t)operation(address, data));
        if (blocking)
        {
            _mtb_hal_flash_clear_cache(clearCache);
        }
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_is_operation_complete
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE bool _mtb_hal_nvm_is_operation_complete(void)
{
    bool complete = true;

    complete = (CY_FLASH_DRV_SUCCESS == Cy_Flash_IsOperationComplete());
    if (complete)
    {
        _mtb_hal_flash_clear_cache(true);
    }

    return complete;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_read_helper_flash
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_nvm_read_helper_flash(uint32_t address, uint8_t* data,
                                                         size_t size)
{
    cy_stc_flash_blankcheck_config_t blankcheck_config;
    blankcheck_config.addrToBeChecked       = (uint32_t*)address;
    blankcheck_config.numOfWordsToBeChecked = size / 4;
    cy_en_flashdrv_status_t blank_check_status = Cy_Flash_BlankCheck(&blankcheck_config,
                                                                     CY_FLASH_DRIVER_BLOCKING);
    /* eCT flash does not have a deterministic erase value, If user read erased region, return all 0
       instead of garbage. */
    if (blank_check_status == CY_FLASH_DRV_SUCCESS)
    {
        memset((void*)data, 0, size);
    }
    else
    {
        /* If CBUS with icache, we always use CBUS to read flash memory in order to take advantage
           of icache property. */
        #if defined(CY_FLASH_CBUS_BASE) && defined(CY_IP_M33SYSCPUSS)
        memcpy((void*)data, (void*)(address & ~SBUS_ALIAS_OFFSET), size);
        #else
        memcpy((void*)data, (void*)address, size);
        #endif
    }
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_erase_helper_flash
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_nvm_erase_helper_flash(uint32_t address, bool blocking)
{
    /* ECT Flash supports sector-based erase operations */
    cy_rslt_t status;
    uint32_t pdl_status;

    if (blocking)
    {
        pdl_status = (uint32_t)Cy_Flash_EraseSector(address);
        _mtb_hal_flash_clear_cache(true);
    }
    else
    {
        pdl_status = (uint32_t)Cy_Flash_StartEraseSector(address);
    }
    status = (cy_rslt_t)_mtb_hal_flash_convert_status(pdl_status);
    return status;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_write_helper_flash
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_nvm_write_helper_flash(uint32_t address, const uint32_t* data,
                                                          bool blocking)
{
    /* CY_IP_M7CPUSS and CY_IP_MXS40FLASHC do not support a bundled write (erase + program)
     * operation. Instead it is necessary to perform an erase operation followed by a program
     * operation.
     */
    CY_UNUSED_PARAMETER(address);
    CY_UNUSED_PARAMETER(data);
    CY_UNUSED_PARAMETER(blocking);
    return MTB_HAL_NVM_RSLT_ERR_NOT_SUPPORTED;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_program_helper_flash
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_nvm_program_helper_flash(uint32_t address, const uint32_t* data,
                                                            bool blocking)
{
    _mtb_hal_flash_operation operation;

    if (blocking)
    {
        operation = Cy_Flash_ProgramRow;
    }
    else
    {
        operation = Cy_Flash_StartWrite;
    }
    return _mtb_hal_flash_run_operation(operation, address, data, true, blocking);
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_is_blank_helper_flash
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE bool _mtb_hal_nvm_is_blank_helper_flash(uint32_t address, uint32_t size)
{
    /* Uses the SROM IPC blank-check path, which never accesses the flash through the CPU bus
     * and is therefore safe on partially erased eCT cells that would raise a bus/ECC fault. */
    cy_stc_flash_blankcheck_config_t blankcheck_config;
    blankcheck_config.addrToBeChecked       = (const uint32_t*)address;
    blankcheck_config.numOfWordsToBeChecked = size / 4U;
    return (Cy_Flash_BlankCheck(&blankcheck_config,
                                CY_FLASH_DRIVER_BLOCKING) == CY_FLASH_DRV_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_nvm_is_sector_corrupt_helper_flash (implementation notes: see \ref group_hal_impl_nvm)
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE bool _mtb_hal_nvm_is_sector_corrupt_helper_flash(uint32_t address, uint32_t size)
{
    #if (defined(CPUSS_FLASHC_ECT) && (CPUSS_FLASHC_ECT == 1))
    /* The mask math below places each source in FAULT set 1 / MASK1 by (index - 32); verify at
     * compile time that these fault-source indices really are in the set-1 range (32..63). */
    static_assert(((uint32_t)CY_SYSFAULT_FLASHC_WORK_BUS_ERROR >= 32U)
                  && ((uint32_t)CY_SYSFAULT_FLASHC_WORK_BUS_ERROR < 64U),
                  "CY_SYSFAULT_FLASHC_WORK_BUS_ERROR must be in FAULT set 1 (index 32..63)");
    static_assert(((uint32_t)CY_SYSFAULT_FLASHC_WORK_NC_ECC >= 32U)
                  && ((uint32_t)CY_SYSFAULT_FLASHC_WORK_NC_ECC < 64U),
                  "CY_SYSFAULT_FLASHC_WORK_NC_ECC must be in FAULT set 1 (index 32..63)");
    static_assert(((uint32_t)CY_SYSFAULT_RAMC0_NC_ECC >= 32U)
                  && ((uint32_t)CY_SYSFAULT_RAMC0_NC_ECC < 64U),
                  "CY_SYSFAULT_RAMC0_NC_ECC must be in FAULT set 1 (index 32..63)");

    /* Blank eCT work flash has no valid ECC code word, so reading it through the CPU bus raises a
     * WORK_NC_ECC fault even though the sector is not corrupt. A blank sector is not corrupt, and
     * the blank check (SROM) never touches the CPU bus, so use it to skip the read probe. */
    if (_mtb_hal_nvm_is_blank_helper_flash(address, size))
    {
        return false;
    }

    /* All handled sources have index >= 32 -> FAULT_MASK1 / PENDING set 1. */
    static const cy_en_SysFault_source_t our_sources[] =
    {
        (cy_en_SysFault_source_t)CY_SYSFAULT_FLASHC_WORK_BUS_ERROR,
        (cy_en_SysFault_source_t)CY_SYSFAULT_FLASHC_WORK_NC_ECC,
        (cy_en_SysFault_source_t)CY_SYSFAULT_RAMC0_NC_ECC,
    };
    const uint32_t num_sources = (uint32_t)(sizeof(our_sources) / sizeof(our_sources[0]));

    uint32_t our_mask = 0U;
    for (uint32_t i = 0U; i < num_sources; i++)
    {
        /* -32: SET1/MASK1 covers fault IDs 32..63, so the bit position is (id - 32). */
        our_mask |= (1UL << ((uint32_t)our_sources[i] - 32U));
    }

    bool app_captured = (Cy_SysFault_GetErrorSource(FAULT_STRUCT0) != CY_SYSFAULT_NO_FAULT);
    uint32_t saved_mask = FAULT_MASK1(FAULT_STRUCT0);

    /* Enable capture for our sources; unless an application fault is already captured, clear
     * STATUS after each so a stale boot-time pending is drained instead of being misread as
     * corruption on the first probe after a reset. */
    for (uint32_t i = 0U; i < num_sources; i++)
    {
        Cy_SysFault_SetMaskByIdx(FAULT_STRUCT0, our_sources[i]);
        if (!app_captured)
        {
            Cy_SysFault_ClearStatus(FAULT_STRUCT0);
        }
    }

    uint32_t pending_before =
        Cy_SysFault_GetPendingFault(FAULT_STRUCT0, CY_SYSFAULT_SET1) & our_mask;

    /* Silence the work-flash R-bus error so a faulting word is recorded, not CPU-faulted.
     * Invalidate the CM7 D-cache so the probe read reaches the flash array. */
    FLASHC_FLASH_CTL |= FLASHC_FLASH_CTL_WORK_ERR_SILENT_Msk;
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_InvalidateDCache_by_Addr((volatile void*)address, (int32_t)size);
    #endif

    volatile const uint32_t* p_word = (volatile const uint32_t*)address;
    uint32_t dummy_read = 0U;
    for (uint32_t w = 0U; w < (size / 4U); w++)
    {
        dummy_read |= p_word[w];
    }
    (void)dummy_read;

    FLASHC_FLASH_CTL &= ~FLASHC_FLASH_CTL_WORK_ERR_SILENT_Msk;

    uint32_t pending_new =
        (Cy_SysFault_GetPendingFault(FAULT_STRUCT0, CY_SYSFAULT_SET1) & our_mask) & ~pending_before;

    bool is_corrupt;
    if (!app_captured)
    {
        cy_en_SysFault_source_t src = Cy_SysFault_GetErrorSource(FAULT_STRUCT0);
        bool status_hit = false;
        for (uint32_t i = 0U; i < num_sources; i++)
        {
            if (src == our_sources[i])
            {
                status_hit = true;
                break;
            }
        }
        is_corrupt = status_hit || (pending_new != 0U);
        if (status_hit)
        {
            Cy_SysFault_ClearStatus(FAULT_STRUCT0);
        }
    }
    else
    {
        is_corrupt = (pending_new != 0U);
    }

    FAULT_MASK1(FAULT_STRUCT0) = saved_mask;

    return is_corrupt;
    #else /* (CPUSS_FLASHC_ECT == 1) */
    /* Non-eCT flash has no per-word work-flash ECC, so the interrupted-erase ECC corruption
     * this check targets cannot occur. */
    CY_UNUSED_PARAMETER(address);
    CY_UNUSED_PARAMETER(size);
    return false;
    #endif /* (CPUSS_FLASHC_ECT == 1) */
}


#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* (MTB_HAL_DRIVER_AVAILABLE_NVM) */
