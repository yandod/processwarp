; ModuleID = 'test_llrint.bc'
target datalayout = "e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [21 x i8] c"%lld\0A%lld\0A%lld\0A%lld\0A\00", align 1

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = tail call i64 @llrint(double 1.000000e-01) #2
  %2 = tail call i64 @llrint(double 6.000000e-01) #2
  %3 = tail call i64 @llrint(double 1.250000e+00) #2
  %4 = tail call i64 @llrint(double 0x4270000000000AAC) #2
  %5 = tail call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([21 x i8]* @.str, i64 0, i64 0), i64 %1, i64 %2, i64 %3, i64 %4) #2
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @printf(i8* nocapture readonly, ...) #1

; Function Attrs: nounwind
declare i64 @llrint(double) #1

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.4-1ubuntu3 (tags/RELEASE_34/final) (based on LLVM 3.4)"}
