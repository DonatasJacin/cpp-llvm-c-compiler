; ModuleID = 'mini-c'
source_filename = "mini-c"

define i32 @factorial(i32 %n) {
entry:
  %factorial = alloca i32, align 4
  %i = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, i32* %n1, align 4
  store i32 1, i32* %factorial, align 4
  store i32 1, i32* %i, align 4
  br label %header

header:                                           ; preds = %body, %entry
  %i2 = load i32, i32* %i, align 4
  %n3 = load i32, i32* %n1, align 4
  %sletmp = icmp sle i32 %i2, %n3
  %whilecond = icmp ne i1 %sletmp, false
  br i1 %whilecond, label %body, label %end

body:                                             ; preds = %header
  %factorial4 = load i32, i32* %factorial, align 4
  %i5 = load i32, i32* %i, align 4
  %multmp = mul i32 %factorial4, %i5
  store i32 %multmp, i32* %factorial, align 4
  %i6 = load i32, i32* %i, align 4
  %addtmp = add i32 %i6, 1
  store i32 %addtmp, i32* %i, align 4
  br label %header
  br label %end

end:                                              ; preds = %body, %header
  %factorial7 = load i32, i32* %factorial, align 4
  ret i32 %factorial7
}
