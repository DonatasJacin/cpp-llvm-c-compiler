; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addNumbers(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, i32* %n1, align 4
  store i32 0, i32* %result, align 4
  %n2 = load i32, i32* %n1, align 4
  %netmp = icmp ne i32 %n2, 0
  %ifcond = icmp ne i1 %netmp, false
  br i1 %ifcond, label %"if then", label %"else then"

"if then":                                        ; preds = %entry
  %n3 = load i32, i32* %n1, align 4
  %n4 = load i32, i32* %n1, align 4
  %subtmp = sub i32 %n4, 1
  %calltmp = call i32 @addNumbers(i32 %subtmp)
  %addtmp = add i32 %n3, %calltmp
  store i32 %addtmp, i32* %result, align 4
  br label %end

"else then":                                      ; preds = %entry
  %n5 = load i32, i32* %n1, align 4
  store i32 %n5, i32* %result, align 4
  br label %end

end:                                              ; preds = %"else then", %"if then"
  %result6 = load i32, i32* %result, align 4
  %calltmp7 = call i32 @print_int(i32 %result6)
  %result8 = load i32, i32* %result, align 4
  ret i32 %result8
}

define i32 @recursion_driver(i32 %num) {
entry:
  %num1 = alloca i32, align 4
  store i32 %num, i32* %num1, align 4
  %num2 = load i32, i32* %num1, align 4
  %calltmp = call i32 @addNumbers(i32 %num2)
  ret i32 %calltmp
}
