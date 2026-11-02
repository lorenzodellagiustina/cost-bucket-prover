; ModuleID = '/tmp/tmp.SRLz2WwNLd/linked.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@__hux_cost = global i64 0

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @loop3(i32 noundef %0) #0 {
  %hux.cost = load i64, i64* @__hux_cost, align 8
  %2 = add i64 %hux.cost, 7
  store i64 %2, i64* @__hux_cost, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 0, i32* %4, align 4
  store i32 0, i32* %5, align 4
  br label %6

6:                                                ; preds = %16, %1
  %hux.cost1 = load i64, i64* @__hux_cost, align 8
  %7 = add i64 %hux.cost1, 4
  store i64 %7, i64* @__hux_cost, align 8
  %8 = load i32, i32* %5, align 4
  %9 = load i32, i32* %3, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %20

11:                                               ; preds = %6
  %hux.cost2 = load i64, i64* @__hux_cost, align 8
  %12 = add i64 %hux.cost2, 5
  store i64 %12, i64* @__hux_cost, align 8
  %13 = load i32, i32* %5, align 4
  %14 = load i32, i32* %4, align 4
  %15 = add nsw i32 %14, %13
  store i32 %15, i32* %4, align 4
  br label %16

16:                                               ; preds = %11
  %hux.cost3 = load i64, i64* @__hux_cost, align 8
  %17 = add i64 %hux.cost3, 4
  store i64 %17, i64* @__hux_cost, align 8
  %18 = load i32, i32* %5, align 4
  %19 = add nsw i32 %18, 1
  store i32 %19, i32* %5, align 4
  br label %6, !llvm.loop !6

20:                                               ; preds = %6
  %hux.cost4 = load i64, i64* @__hux_cost, align 8
  %21 = add i64 %hux.cost4, 2
  store i64 %21, i64* @__hux_cost, align 8
  %22 = load i32, i32* %4, align 4
  ret i32 %22
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.ident = !{!0}
!llvm.module.flags = !{!1, !2, !3, !4, !5}

!0 = !{!"Ubuntu clang version 14.0.0-1ubuntu1"}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"PIC Level", i32 2}
!3 = !{i32 7, !"PIE Level", i32 2}
!4 = !{i32 7, !"uwtable", i32 1}
!5 = !{i32 7, !"frame-pointer", i32 2}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
