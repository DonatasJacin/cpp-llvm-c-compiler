; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define void @addition(i32 %n, i32 %m) {
entry:
  %skeng = alloca float, align 4
  %z = alloca i32, align 4
  %result = alloca i32, align 4
  %m2 = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, i32* %n1, align 4
  store i32 %m, i32* %m2, align 4
  %n3 = load i32, i32* %n1, align 4
  %m4 = load i32, i32* %m2, align 4
  %addtmp = add i32 %n3, %m4
  store i32 %addtmp, i32* %result, align 4
  %n5 = load i32, i32* %n1, align 4
  %eqtmp = icmp eq i32 %n5, 4
  %ifcond = icmp ne i1 %eqtmp, false
  br i1 %ifcond, label %"if then", label %"else then"

"if then":                                        ; preds = %entry
  %n6 = load i32, i32* %n1, align 4
  %m7 = load i32, i32* %m2, align 4
  %addtmp8 = add i32 %n6, %m7
  %calltmp = call i32 @print_int(i32 %addtmp8)
  br label %end

"else then":                                      ; preds = %entry
  %n9 = load i32, i32* %n1, align 4
  %m10 = load i32, i32* %m2, align 4
  %multmp = mul i32 %n9, %m10
  %calltmp11 = call i32 @print_int(i32 %multmp)
  br label %end

end:                                              ; preds = %"else then", %"if then"
  ret void
  ret void
}
