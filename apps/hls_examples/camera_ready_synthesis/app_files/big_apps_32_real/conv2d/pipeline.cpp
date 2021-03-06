#include "Halide.h"
#include <stdio.h>

using namespace Halide;

Var x("x"), y("y"), c("c");
Var xo("xo"), yo("yo"), xi("xi"), yi("yi"), xii("xii"), yii("yii");

class MyPipeline {
public:
    ImageParam input;
    Func output, hw_input, hw_output, mul, avg, kernel;
    RDom win;
    std::vector<Argument> args;

    MyPipeline()
        : input(UInt(32), 2),
          output("output"),
          hw_input("hw_input"),
          hw_output("hw_output"),
          mul("mul"),
          avg("avg"),
          win(0, 3, 0, 3)
    {

      kernel(x, y) = cast<uint32_t>(1);

      kernel(0, 0) = cast<uint32_t>(3);
      kernel(1, 0) = cast<uint32_t>(5);
      kernel(2, 0) = cast<uint32_t>(3);

      kernel(0, 1) = cast<uint32_t>(5);
      kernel(1, 1) = cast<uint32_t>(7);
      kernel(2, 1) = cast<uint32_t>(3);

      kernel(0, 2) = cast<uint32_t>(3);
      kernel(1, 2) = cast<uint32_t>(5);
      kernel(2, 2) = cast<uint32_t>(3);
      
      kernel.bound(x, 0, 3);
      kernel.bound(y, 0, 3);

      hw_input(x, y) = input(x, y);
      mul(x, y) = 0;
      mul(x, y) += cast<uint32_t>(hw_input(win.x + x, win.y + y)) * cast<uint32_t>(kernel(win.x, win.y));
      //avg(x, y) = mul(x, y) >> 4;
      //avg(x, y) = cast<uint32_t>(cast<float>(mul(x, y)) * Expr(1.0 / 16.0));
      
      //avg(x, y) = cast<uint32_t>((cast<uint64_t>(mul(x, y)) * Expr(16)) >> 8);
      avg(x, y) = cast<uint32_t>(mul(x, y) / 39);

      hw_output(x, y) = cast<uint32_t>(avg(x, y));
      output(x, y) = cast<uint32_t>(hw_output(x, y));

      args = {input};
    }

    void compile_hls() {
        std::cout << "\ncompiling HLS code..." << std::endl;

        hw_input.compute_root();
        hw_output.compute_root();

        hw_output.tile(x, y, xo, yo, xi, yi, 1920 - 2, 1080 - 2)
          .reorder(xi, yi, xo, yo)
          ;
        hw_output.accelerate({hw_input}, xi, xo);
        kernel.compute_at(hw_output, xo).unroll(x).unroll(y);

        mul.update(0)
          .unroll(win.x).unroll(win.y);

        output.print_loop_nest();

        // Create the target for HLS simulation
        Target hls_target = get_target_from_environment();
        hls_target.set_feature(Target::CPlusPlusMangling);
        output.compile_to_lowered_stmt("pipeline_hls.ir.html", args, HTML, hls_target);
        output.compile_to_hls("pipeline_hls.cpp", args, "pipeline_hls", hls_target);
        output.compile_to_header("pipeline_hls.h", args, "pipeline_hls", hls_target);

        //std::vector<Target::Feature> features({Target::Zynq});
        //Target target(Target::Linux, Target::ARM, 32, features);
        //output.compile_to_zynq_c("pipeline_zynq.c", args, "pipeline_zynq", target);
        //output.compile_to_header("pipeline_zynq.h", args, "pipeline_zynq", target);
        //output.compile_to_lowered_stmt("pipeline_zynq.ir.html", args, HTML, target);
    }
};

int main(int argc, char **argv) {
    MyPipeline p2;
    p2.compile_hls();

    return 0;
}
