; Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
;
; Module Name:
;
;   ReadCr4.Asm
;
; Abstract:
;
;   AsmReadCr4 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadCr4 (
;   VOID
;   );
;------------------------------------------------------------------------------
AsmReadCr4  PROC
    mov     rax, cr4
    ret
AsmReadCr4  ENDP

    END
