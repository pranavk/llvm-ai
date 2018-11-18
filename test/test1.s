; ModuleID = 'test1.s'
source_filename = "test1.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline norecurse nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = add nsw i32 42, 42
  %2 = add nsw i32 %1, %1
  %3 = sub nsw i32 %1, %1
  %4 = sub nsw i32 %2, %3
  %5 = add nsw i32 %4, %1
  ret i32 %5
}

attributes #0 = { noinline norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 7.0.1 (git@github.com:llvm-mirror/clang.git 0513b409d5e34b2d2a28ae21b6d620cc52de0e57) (git@github.com:llvm-mirror/llvm.git d0abf8be7d16d63c025fb9709404ee865d2acc1a)"}
