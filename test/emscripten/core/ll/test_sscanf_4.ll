; ModuleID = 'test_sscanf_4.bc'
target datalayout = "e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@.str1 = private unnamed_addr constant [12 x i8] c"Nov 19 2012\00", align 1
@.str2 = private unnamed_addr constant [7 x i8] c"%s%s%s\00", align 1
@.str3 = private unnamed_addr constant [28 x i8] c"day %s, month %s, year %s \0A\00", align 1

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %pYear = alloca [16 x i8], align 16
  %pMonth = alloca [16 x i8], align 16
  %pDay = alloca [16 x i8], align 16
  %1 = getelementptr inbounds [16 x i8]* %pMonth, i64 0, i64 0
  %2 = getelementptr inbounds [16 x i8]* %pDay, i64 0, i64 0
  %3 = getelementptr inbounds [16 x i8]* %pYear, i64 0, i64 0
  %4 = call i32 (i8*, i8*, ...)* @__isoc99_sscanf(i8* getelementptr inbounds ([12 x i8]* @.str1, i64 0, i64 0), i8* getelementptr inbounds ([7 x i8]* @.str2, i64 0, i64 0), i8* %1, i8* %2, i8* %3) #2
  %5 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i64 0, i64 0), i32 %4) #2
  %6 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([28 x i8]* @.str3, i64 0, i64 0), i8* %2, i8* %1, i8* %3) #2
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @printf(i8* nocapture readonly, ...) #1

; Function Attrs: nounwind
declare i32 @__isoc99_sscanf(i8* nocapture readonly, i8* nocapture readonly, ...) #1

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.4-1ubuntu3 (tags/RELEASE_34/final) (based on LLVM 3.4)"}
