; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define float @unary(i32 %n, float %m) {
entry:
  %sum = alloca float, align 4
  %result = alloca float, align 4
  %m2 = alloca float, align 4
  %n1 = alloca i32, align 4
  store i32 %n, i32* %n1, align 4
  store float %m, float* %m2, align 4
  store float 0.000000e+00, float* %sum, align 4
  %n3 = load i32, i32* %n1, align 4
  %m4 = load float, float* %m2, align 4
  %convtmp = sitofp i32 %n3 to float
  %addftmp = fadd float %convtmp, %m4
  store float %addftmp, float* %result, align 4
  %result5 = load float, float* %result, align 4
  %calltmp = call float @print_float(float %result5)
  %sum6 = load float, float* %sum, align 4
  %result7 = load float, float* %result, align 4
  %addftmp8 = fadd float %sum6, %result7
  store float %addftmp8, float* %sum, align 4
  %n9 = load i32, i32* %n1, align 4
  %m10 = load float, float* %m2, align 4
  %negftmp = fneg float %m10
  %convtmp11 = sitofp i32 %n9 to float
  %addftmp12 = fadd float %convtmp11, %negftmp
  store float %addftmp12, float* %result, align 4
  %result13 = load float, float* %result, align 4
  %calltmp14 = call float @print_float(float %result13)
  %sum15 = load float, float* %sum, align 4
  %result16 = load float, float* %result, align 4
  %addftmp17 = fadd float %sum15, %result16
  store float %addftmp17, float* %sum, align 4
  %n18 = load i32, i32* %n1, align 4
  %m19 = load float, float* %m2, align 4
  %negftmp20 = fneg float %m19
  %negftmp21 = fneg float %negftmp20
  %convtmp22 = sitofp i32 %n18 to float
  %addftmp23 = fadd float %convtmp22, %negftmp21
  store float %addftmp23, float* %result, align 4
  %result24 = load float, float* %result, align 4
  %calltmp25 = call float @print_float(float %result24)
  %sum26 = load float, float* %sum, align 4
  %result27 = load float, float* %result, align 4
  %addftmp28 = fadd float %sum26, %result27
  store float %addftmp28, float* %sum, align 4
  %n29 = load i32, i32* %n1, align 4
  %negtmp = sub i32 0, %n29
  %m30 = load float, float* %m2, align 4
  %negftmp31 = fneg float %m30
  %convtmp32 = sitofp i32 %negtmp to float
  %addftmp33 = fadd float %convtmp32, %negftmp31
  store float %addftmp33, float* %result, align 4
  %result34 = load float, float* %result, align 4
  %calltmp35 = call float @print_float(float %result34)
  %sum36 = load float, float* %sum, align 4
  %result37 = load float, float* %result, align 4
  %addftmp38 = fadd float %sum36, %result37
  store float %addftmp38, float* %sum, align 4
  %sum39 = load float, float* %sum, align 4
  ret float %sum39
}
