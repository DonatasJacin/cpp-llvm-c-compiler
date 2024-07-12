; ModuleID = 'mini-c'
source_filename = "mini-c"

define i1 @palindrome(i32 %number) {
entry:
  %result = alloca i1, align 1
  %rmndr = alloca i32, align 4
  %rev = alloca i32, align 4
  %t = alloca i32, align 4
  %number1 = alloca i32, align 4
  store i32 %number, i32* %number1, align 4
  store i32 0, i32* %rev, align 4
  store i1 false, i1* %result, align 1
  %number2 = load i32, i32* %number1, align 4
  store i32 %number2, i32* %t, align 4
  br label %header

header:                                           ; preds = %body, %entry
  %number3 = load i32, i32* %number1, align 4
  %sgttmp = icmp sgt i32 %number3, 0
  %whilecond = icmp ne i1 %sgttmp, false
  br i1 %whilecond, label %body, label %end

body:                                             ; preds = %header
  %number4 = load i32, i32* %number1, align 4
  %remtmp = urem i32 %number4, 10
  store i32 %remtmp, i32* %rmndr, align 4
  %rev5 = load i32, i32* %rev, align 4
  %multmp = mul i32 %rev5, 10
  %rmndr6 = load i32, i32* %rmndr, align 4
  %addtmp = add i32 %multmp, %rmndr6
  store i32 %addtmp, i32* %rev, align 4
  %number7 = load i32, i32* %number1, align 4
  %divtmp = sdiv i32 %number7, 10
  store i32 %divtmp, i32* %number1, align 4
  br label %header
  br label %end

end:                                              ; preds = %body, %header
  %t8 = load i32, i32* %t, align 4
  %rev9 = load i32, i32* %rev, align 4
  %eqtmp = icmp eq i32 %t8, %rev9
  %ifcond = icmp ne i1 %eqtmp, false
  br i1 %ifcond, label %"if then", label %"else then"

"if then":                                        ; preds = %end
  store i1 true, i1* %result, align 1
  br label %end10

"else then":                                      ; preds = %end
  store i1 false, i1* %result, align 1
  br label %end10

end10:                                            ; preds = %"else then", %"if then"
  %result11 = load i1, i1* %result, align 1
  ret i1 %result11
}
