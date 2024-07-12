; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @pi() {
entry:
  %i = alloca i32, align 4
  %PI = alloca float, align 4
  %flag = alloca i1, align 1
  store i1 true, i1* %flag, align 1
  store float 3.000000e+00, float* %PI, align 4
  store i32 2, i32* %i, align 4
  br label %header

header:                                           ; preds = %end, %entry
  %i1 = load i32, i32* %i, align 4
  %slttmp = icmp slt i32 %i1, 100
  %whilecond = icmp ne i1 %slttmp, false
  br i1 %whilecond, label %body, label %end22

body:                                             ; preds = %header
  %flag2 = load i1, i1* %flag, align 1
  %ifcond = icmp ne i1 %flag2, false
  br i1 %ifcond, label %"if then", label %"else then"

"if then":                                        ; preds = %body
  %PI3 = load float, float* %PI, align 4
  %i4 = load i32, i32* %i, align 4
  %i5 = load i32, i32* %i, align 4
  %addtmp = add i32 %i5, 1
  %multmp = mul i32 %i4, %addtmp
  %i6 = load i32, i32* %i, align 4
  %addtmp7 = add i32 %i6, 2
  %multmp8 = mul i32 %multmp, %addtmp7
  %convtmp = sitofp i32 %multmp8 to float
  %divftmp = fdiv float 4.000000e+00, %convtmp
  %addftmp = fadd float %PI3, %divftmp
  store float %addftmp, float* %PI, align 4
  br label %end

"else then":                                      ; preds = %body
  %PI9 = load float, float* %PI, align 4
  %i10 = load i32, i32* %i, align 4
  %i11 = load i32, i32* %i, align 4
  %addtmp12 = add i32 %i11, 1
  %multmp13 = mul i32 %i10, %addtmp12
  %i14 = load i32, i32* %i, align 4
  %addtmp15 = add i32 %i14, 2
  %multmp16 = mul i32 %multmp13, %addtmp15
  %convtmp17 = sitofp i32 %multmp16 to float
  %divftmp18 = fdiv float 4.000000e+00, %convtmp17
  %subftmp = fsub float %PI9, %divftmp18
  store float %subftmp, float* %PI, align 4
  br label %end

end:                                              ; preds = %"else then", %"if then"
  %flag19 = load i1, i1* %flag, align 1
  %nottmp = xor i1 %flag19, true
  store i1 %nottmp, i1* %flag, align 1
  %i20 = load i32, i32* %i, align 4
  %addtmp21 = add i32 %i20, 2
  store i32 %addtmp21, i32* %i, align 4
  br label %header
  br label %end22

end22:                                            ; preds = %end, %header
  %PI23 = load float, float* %PI, align 4
  ret float %PI23
}
