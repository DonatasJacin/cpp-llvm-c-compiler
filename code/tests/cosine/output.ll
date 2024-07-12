; ModuleID = 'mini-c'
source_filename = "mini-c"

declare float @print_float(float)

define float @cosine(float %x) {
entry:
  %alt = alloca float, align 4
  %eps = alloca float, align 4
  %term = alloca float, align 4
  %n = alloca float, align 4
  %cos = alloca float, align 4
  %x1 = alloca float, align 4
  store float %x, float* %x1, align 4
  store float 0x3EB0C6F7A0000000, float* %eps, align 4
  store float 1.000000e+00, float* %n, align 4
  store float 1.000000e+00, float* %cos, align 4
  store float 1.000000e+00, float* %term, align 4
  store float -1.000000e+00, float* %alt, align 4
  br label %header

header:                                           ; preds = %body, %entry
  %term2 = load float, float* %term, align 4
  %eps3 = load float, float* %eps, align 4
  %sgtftmp = fcmp ugt float %term2, %eps3
  %whilecond = icmp ne i1 %sgtftmp, false
  br i1 %whilecond, label %body, label %end

body:                                             ; preds = %header
  %term4 = load float, float* %term, align 4
  %x5 = load float, float* %x1, align 4
  %mulftmp = fmul float %term4, %x5
  %x6 = load float, float* %x1, align 4
  %mulftmp7 = fmul float %mulftmp, %x6
  %n8 = load float, float* %n, align 4
  %divftmp = fdiv float %mulftmp7, %n8
  %n9 = load float, float* %n, align 4
  %addftmp = fadd float %n9, 1.000000e+00
  %divftmp10 = fdiv float %divftmp, %addftmp
  store float %divftmp10, float* %term, align 4
  %cos11 = load float, float* %cos, align 4
  %alt12 = load float, float* %alt, align 4
  %term13 = load float, float* %term, align 4
  %mulftmp14 = fmul float %alt12, %term13
  %addftmp15 = fadd float %cos11, %mulftmp14
  store float %addftmp15, float* %cos, align 4
  %alt16 = load float, float* %alt, align 4
  %negftmp = fneg float %alt16
  store float %negftmp, float* %alt, align 4
  %n17 = load float, float* %n, align 4
  %addftmp18 = fadd float %n17, 2.000000e+00
  store float %addftmp18, float* %n, align 4
  br label %header
  br label %end

end:                                              ; preds = %body, %header
  %cos19 = load float, float* %cos, align 4
  %calltmp = call float @print_float(float %cos19)
  %cos20 = load float, float* %cos, align 4
  ret float %cos20
}
