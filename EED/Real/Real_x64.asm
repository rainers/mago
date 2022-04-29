;; ml64 Real_x64.asm

.code

;; public: void __cdecl Real10::FromDouble(double)
?FromDouble@Real10@@QEAAXN@Z proc
        sub rsp, 8
        movsd qword ptr [rsp],xmm1
        fld qword ptr [rsp]
        fstp tbyte ptr [rcx]
        add rsp, 8
        ret
?FromDouble@Real10@@QEAAXN@Z endp

;; public: void __cdecl Real10::FromFloat(float)
?FromFloat@Real10@@QEAAXM@Z proc
        sub rsp, 8
        movss dword ptr [rsp],xmm1
        fld dword ptr [rsp]
        fstp tbyte ptr [rcx]
        add rsp, 8
        ret
?FromFloat@Real10@@QEAAXM@Z endp

;; public: void __cdecl Real10::FromInt64(__int64)
?FromInt64@Real10@@QEAAX_J@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        mov qword ptr [rsp],rdx
        fild qword ptr [rsp]
        fstp tbyte ptr [rcx]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?FromInt64@Real10@@QEAAX_J@Z endp

;; public: void __cdecl Real10::FromInt32(int)
?FromInt32@Real10@@QEAAXH@Z proc
        sub rsp, 8
        mov dword ptr [rsp],edx
        fild dword ptr [rsp]
        fstp tbyte ptr [rcx]
        add rsp, 8
        ret
?FromInt32@Real10@@QEAAXH@Z endp

;; public: void __cdecl Real10::FromUInt64(unsigned __int64)
?FromUInt64@Real10@@QEAAX_K@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        mov rax,1 shl 63
        xor rdx, rax
        mov qword ptr [rsp],rdx
        fild qword ptr [rsp]
        mov qword ptr [rsp], rax
        fild qword ptr [rsp]           ; -(1 << 63)
        fsubp ST(1),ST(0)
        fstp tbyte ptr [rcx]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?FromUInt64@Real10@@QEAAX_K@Z endp

;; public: double __cdecl Real10::ToDouble(void)const 
?ToDouble@Real10@@QEBANXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fstp qword ptr [rsp]
        movsd xmm0, qword ptr [rsp]
        add rsp, 8
        ret
?ToDouble@Real10@@QEBANXZ endp

;; public: float __cdecl Real10::ToFloat(void)const
?ToFloat@Real10@@QEBAMXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fstp dword ptr [rsp]
        movss xmm0, dword ptr [rsp]
        add rsp, 8
        ret
?ToFloat@Real10@@QEBAMXZ endp

;; public: short __cdecl Real10::ToInt16(void)const
?ToInt16@Real10@@QEBAFXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fistp word ptr [rsp]
        mov ax, word ptr [rsp]
        add rsp, 8
        ret
?ToInt16@Real10@@QEBAFXZ endp

;; public: int __cdecl Real10::ToInt32(void)const
?ToInt32@Real10@@QEBAHXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fistp dword ptr [rsp]
        mov eax, dword ptr [rsp]
        add rsp, 8
        ret
?ToInt32@Real10@@QEBAHXZ endp

;; public: __int64 __cdecl Real10::ToInt64(void)const
?ToInt64@Real10@@QEBA_JXZ proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        fld tbyte ptr [rcx]
        fistp qword ptr [rsp]
        mov rax, qword ptr [rsp]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?ToInt64@Real10@@QEBA_JXZ endp

;; public: unsigned __int64 __cdecl Real10::ToUInt64(void)const
?ToUInt64@Real10@@QEBA_KXZ proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        mov rax,1 shl 63
        mov qword ptr [rsp], rax
        fld tbyte ptr [rcx]
        fild qword ptr [rsp]
        faddp ST(1),ST(0)
        fistp qword ptr [rsp]
        xor rax, qword ptr [rsp]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?ToUInt64@Real10@@QEBA_KXZ endp

;; public: bool __cdecl Real10::FitsInDouble(void)const
?FitsInDouble@Real10@@QEBA_NXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fxam
        fnstsw ax
        mov dx, ax
        fstp qword ptr [rsp]
        fld qword ptr [rsp]
        fxam
        fnstsw word ptr ax
        fstp ST(0)
        xor ax, dx
        test ax, 4500h
        setz al
        add rsp, 8
        ret
?FitsInDouble@Real10@@QEBA_NXZ endp

;; public: bool __cdecl Real10::FitsInFloat(void)const
?FitsInFloat@Real10@@QEBA_NXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fxam
        fnstsw ax
        mov dx, ax
        fstp dword ptr [rsp]
        fld dword ptr [rsp]
        fxam
        fnstsw word ptr ax
        fstp ST(0)
        xor ax, dx
        test ax, 4500h
        setz al
        add rsp, 8
        ret
?FitsInFloat@Real10@@QEBA_NXZ endp

;; public: bool __cdecl Real10::IsZero(void)const
?IsZero@Real10@@QEBA_NXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        ftst
        fnstsw word ptr [rsp]
        fstp ST(0)
        mov ax, 4000h
        xor ax, word ptr[rsp]
        test ax, 4500h
        setz al
        add rsp, 8
        ret
?IsZero@Real10@@QEBA_NXZ endp

;; public: bool __cdecl Real10::IsNan(void)const
?IsNan@Real10@@QEBA_NXZ proc
        sub rsp, 8
        fld tbyte ptr [rcx]
        fxam
        fnstsw word ptr [rsp]
        fstp ST(0)
        mov ax, 100h
        xor ax, word ptr[rsp]
        test ax, 4500h
        setz al
        add rsp, 8
        ret
?IsNan@Real10@@QEBA_NXZ endp

;; public: static unsigned short __cdecl Real10::Compare(struct Real10 const &,struct Real10 const &)
?Compare@Real10@@SAGAEBU1@0@Z proc
        fld tbyte ptr [rdx]
        fld tbyte ptr [rcx]
        fcompp                  ; compare ST(0) with ST(1) then pop twice
        fnstsw ax
        ret
?Compare@Real10@@SAGAEBU1@0@Z endp


;; public: void __cdecl Real10::Add(struct Real10 const &,struct Real10 const &)
?Add@Real10@@QEAAXAEBU1@0@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        fld tbyte ptr [rdx]
        fld tbyte ptr [r8]
        faddp ST(1), ST(0)
        fstp tbyte ptr [rcx]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?Add@Real10@@QEAAXAEBU1@0@Z endp

;; public: void __cdecl Real10::Sub(struct Real10 const &,struct Real10 const &)
?Sub@Real10@@QEAAXAEBU1@0@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        fld tbyte ptr [rdx]
        fld tbyte ptr [r8]
        fsubp ST(1), ST(0)
        fstp tbyte ptr [rcx]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?Sub@Real10@@QEAAXAEBU1@0@Z endp

;; public: void __cdecl Real10::Mul(struct Real10 const &,struct Real10 const &)
?Mul@Real10@@QEAAXAEBU1@0@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        fld tbyte ptr [rdx]
        fld tbyte ptr [r8]
        fmulp ST(1), ST(0)
        fstp tbyte ptr [rcx]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?Mul@Real10@@QEAAXAEBU1@0@Z endp

;; public: void __cdecl Real10::Div(struct Real10 const &,struct Real10 const &)
?Div@Real10@@QEAAXAEBU1@0@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        fld tbyte ptr [rdx]
        fld tbyte ptr [r8]
        fdivp ST(1), ST(0)
        fstp tbyte ptr [rcx]

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret
?Div@Real10@@QEAAXAEBU1@0@Z endp

;; public: void __cdecl Real10::Rem(struct Real10 const &,struct Real10 const &)
?Rem@Real10@@QEAAXAEBU1@0@Z proc
        sub rsp, 8+16
        fnstcw word ptr [rsp+8]        ; save cw
        fnstcw word ptr [rsp+10]
        or word ptr [rsp+10], 0300h    ; set precision to double-extended
        fldcw word ptr [rsp+10]

        fld tbyte ptr [r8]
        fld tbyte ptr [rdx]
Retry:
        fprem                          ; ST(0) := ST(0) % ST(1)    doesn't pop!
        fnstsw ax
        sahf
        jp Retry
        fstp tbyte ptr [rcx]
        fstp ST(0)

        fldcw word ptr [rsp+8]         ; restore cw
        add rsp, 8+16
        ret

        ret
?Rem@Real10@@QEAAXAEBU1@0@Z endp

;; public: void __cdecl Real10::Negate(struct Real10 const &)
?Negate@Real10@@QEAAXAEBU1@@Z proc
        fld tbyte ptr [rdx]
        fchs
        fstp tbyte ptr [rcx]
        ret
?Negate@Real10@@QEAAXAEBU1@@Z endp

;; public: void __cdecl Real10::Abs(struct Real10 const &)
?Abs@Real10@@QEAAXAEBU1@@Z proc
        fld tbyte ptr [rdx]
        fabs
        fstp tbyte ptr [rcx]
        ret
?Abs@Real10@@QEAAXAEBU1@@Z endp

end
