/** @file

  Copyright (c) 2004  - 2016, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



**/

#include <PiDxe.h>

#include <Library/FlashDeviceLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/EventGroup.h>
#include <Library/SpiFlash.H>

#define FLASH_SIZE  0x400000

#define FLASH_DEVICE_BASE_ADDRESS (0xFFFFFFFF-FLASH_SIZE+1)
UINTN FlashDeviceBase = FLASH_DEVICE_BASE_ADDRESS;

EFI_SPI_PROTOCOL *mSpiProtocol = NULL;

EFI_STATUS
SpiFlashErase (
  UINT8 *BaseAddress,
  UINTN NumBytes
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
  UINT32              SectorSize;
  UINT32              SpiAddress;

  SpiAddress = (UINT32)(UINTN)(BaseAddress) - (UINT32)FlashDeviceBase;
  SectorSize = SECTOR_SIZE_4KB;
  while ( (NumBytes > 0) && (NumBytes <= MAX_FWH_SIZE) ) {
    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_SERASE,
                             SPI_WREN,
                             FALSE,
                             TRUE,
                             FALSE,
                             (UINT32) SpiAddress,
                             0,
                             NULL,
                             EnumSpiRegionBios
                             );
    if (EFI_ERROR (Status)) {
      break;
    }
    SpiAddress += SectorSize;
    NumBytes   -= SectorSize;
  }

  return Status;
}


EFI_STATUS
SpiFlashBlockErase (
  UINT8 *BaseAddress,
  UINTN NumBytes
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
  UINT32              SectorSize;
  UINT32              SpiAddress;

  SpiAddress = (UINT32)(UINTN)(BaseAddress) - (UINT32)FlashDeviceBase;
  SectorSize = SECTOR_SIZE_64KB;
  while ( (NumBytes > 0) && (NumBytes <= MAX_FWH_SIZE) ) {
    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_BERASE,
                             SPI_WREN,
                             FALSE,
                             TRUE,
                             FALSE,
                             (UINT32) SpiAddress,
                             0,
                             NULL,
                             EnumSpiRegionBios
                             );
    if (EFI_ERROR (Status)) {
      break;
    }
    SpiAddress += SectorSize;
    NumBytes   -= SectorSize;
  }

  return Status;
}


static
EFI_STATUS
SpiFlashWrite (
  UINT8 *DstBufferPtr,
  UINT8 *Byte,
  IN  UINTN Length
  )
{
  EFI_STATUS                Status;
  UINT32                    NumBytes = (UINT32)Length;
  UINT8*                    pBuf8 = Byte;
  UINT32                    SpiAddress;

  SpiAddress = (UINT32)(UINTN)(DstBufferPtr) - (UINT32)FlashDeviceBase;
  Status = mSpiProtocol->Execute (
                           mSpiProtocol,
                           SPI_PROG,
                           SPI_WREN,
                           TRUE,
                           TRUE,
                           TRUE,
                           (UINT32)SpiAddress,
                           NumBytes,
                           pBuf8,
                           EnumSpiRegionBios
                           );
  return Status;
}

/**
  Read the Serial Flash Status Registers.

  @param  SpiStatus         Pointer to a caller-allocated UINT8. On successful return, it contains the
                            status data read from the Serial Flash Status Register.


  @retval EFI_SUCCESS       Operation success, status is returned in SpiStatus.
  @retval EFI_DEVICE_ERROR  The block device is not functioning correctly and the operation failed.

**/
EFI_STATUS
ReadStatusRegister (
  UINT8   *SpiStatus
  )
{
  EFI_STATUS          Status;

  Status = mSpiProtocol->Execute (
             mSpiProtocol,
             SPI_RDSR,
             SPI_WREN,
             TRUE,
             FALSE,
             FALSE,
             0,
             1,
             SpiStatus,
             EnumSpiRegionBios
             );
  return Status;
}

EFI_STATUS
SpiFlashLock (
    IN  UINT8  *BaseAddress,
    IN  UINTN  NumBytes,
    IN  BOOLEAN  Lock
  )
{
  EFI_STATUS          Status;
  UINT8               SpiData;
  UINT8               SpiStatus;

  if (Lock) {
    SpiData = SF_SR_WPE;
  } else {
    SpiData = 0;
  }

  //
  // Always disable block protection to workaround tool issue.
  // Feature may be re-enabled in a future bios.
  //
  SpiData = 0;
  Status = mSpiProtocol->Execute (
                           mSpiProtocol,
                           SPI_WRSR,
                           SPI_EWSR,
                           TRUE,
                           TRUE,
                           TRUE,
                           0,
                           1,
                           &SpiData,
                           EnumSpiRegionBios
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadStatusRegister (&SpiStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((SpiStatus & SpiData) != SpiData) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}


/**
  Read NumBytes bytes of data from the address specified by
  PAddress into Buffer.

  @param[in]      PAddress          The starting physical address of the read.
  @param[in,out]  NumBytes          On input, the number of bytes to read. On output, the number
                                    of bytes actually read.
  @param[out]     Buffer            The destination data buffer for the read.

  @retval         EFI_SUCCESS.      Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceRead (
  IN      UINTN                           PAddress,
  IN  OUT UINTN                           *NumBytes,
      OUT UINT8                           *Buffer
  )
{
  CopyMem(Buffer, (VOID*)PAddress, *NumBytes);
  return EFI_SUCCESS;
}


/**
  Write NumBytes bytes of data from Buffer to the address specified by
  PAddresss.

  @param[in]      PAddress          The starting physical address of the write.
  @param[in,out]  NumBytes          On input, the number of bytes to write. On output,
                                    the actual number of bytes written.
  @param[in]      Buffer            The source data buffer for the write.

  @retval         EFI_SUCCESS.      Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceWrite (
  IN        UINTN                           PAddress,
  IN OUT    UINTN                           *NumBytes,
  IN        UINT8                           *Buffer
  )
{
EFI_STATUS Status;
  Status =  SpiFlashWrite((UINT8 *)PAddress, Buffer, *NumBytes);
 return Status;
}


/**
  Erase the block staring at PAddress.

  @param[in]  PAddress          The starting physical address of the block to be erased.
                                This library assume that caller garantee that the PAddress
                                is at the starting address of this block.
  @param[in]  LbaLength         The length of the logical block to be erased.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceBlockErase (
  IN    UINTN                     PAddress,
  IN    UINTN                     LbaLength
  )
{
  EFI_STATUS Status;
  Status = SpiFlashBlockErase((UINT8 *)PAddress, LbaLength);

  return Status;
}


/**
  Lock or unlock the block staring at PAddress.

  @param[in]  PAddress        The starting physical address of region to be (un)locked.
  @param[in]  LbaLength       The length of the logical block to be erased.
  @param[in]  Lock            TRUE to lock. FALSE to unlock.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceBlockLock (
  IN    UINTN                          PAddress,
  IN    UINTN                          LbaLength,
  IN    BOOLEAN                        Lock
  )
{
  EFI_STATUS Status;

    Status = SpiFlashLock((UINT8*)PAddress, LbaLength, Lock);
  return Status;
}

