; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define void @Void() {
entry:
  %result = alloca i32, align 4
  store i32 0, i32* %result, align 4
  %result1 = load i32, i32* %result, align 4
  %calltmp = call i32 @print_int(i32 %result1)
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
  %result4 = load i32, i32* %result, align 4
  %calltmp5 = call i32 @print_int(i32 %result4)
  br label %header
  br label %end

end:                                              ; preds = %body, %header
  ret void
  ret void
}
