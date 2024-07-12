; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = common global i32 0, align 4
@f = common global float 0.000000e+00, align 4
@b = common global i1 false, align 4

declare i32 @print_int(i32)

define i32 @While(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, i32* %n1, align 4
  store i32 12, i32* @test, align 4
  store i32 0, i32* %result, align 4
  %test = load i32, i32* @test, align 4
  %calltmp = call i32 @print_int(i32 %test)
  br label %header

header:                                           ; preds = %body, %entry
  %result2 = load i32, i32* %result, align 4
  %slttmp = icmp slt i32 %result2, 10
  %whilecond = icmp ne i1 %slttmp, false
  br i1 %whilecond, label %body, label %end

body:                                             ; preds = %header
  %result3 = load i32, i32* %result, align 4
  %addtmp = add i32 %result3, 1
  store i32 %addtmp, i32* %result, align 4
  br label %header
  br label %end

end:                                              ; preds = %body, %header
  %result4 = load i32, i32* %result, align 4
  ret i32 %result4
}
