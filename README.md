# opencv-object-counting

一个基于 OpenCV 的传统视觉计数实验项目，目标是对规则目标完成分割、轮廓提取、计数和结果标注。

当前分支是 `exp/codex-test-fail`。它不是稳定版本，而是一次 **纯 Codex / AI coding 驱动** 的实验分支，想验证两件事：

1. 让 Codex 主导编码，能把传统视觉方案推进到什么程度。
2. 只靠图像处理和启发式规则，能不能把多类型图像识别做稳。

这次实验最后得到的结论很明确：

**Codex 可以很快把 pipeline 堆出来，但单靠传统图像处理，仍然很难稳定覆盖多类型图像。**

## 这个分支和之前版本的区别

`exp/codex-test-fail` 里其实保留了两条线：

- `Process.cpp` / `main.cpp`
  这是较早的规则链版本，核心是 `judgeBasicType -> basicImg2Binary -> judgeCrave -> craveType -> contoursProcess`。
- `CodexProcess.cpp` / `CodexPipeline_main.cpp`
  这是这个分支真正新增的 Codex 版 pipeline，也是当前 x64 默认编译运行的入口。

也就是说，这个分支不只是“把老规则整理一下”，而是又多做了一套更激进、更工程化的传统视觉方案，专门拿来测试纯 AI coding 能把这条路推多远。

## 当前默认跑的是哪条流程

当前工程在 `Debug|x64` 下默认编译的是：

- `ObjectCounting/CodexPipeline_main.cpp`
- `ObjectCounting/CodexProcess.cpp`
- `ObjectCounting/Process.cpp`

其中：

- `main.cpp` 在 x64 下被排除构建
- `CodexPipeline_main.cpp` 是默认入口
- 输出会写到 `assets/output_codex/`

本机已验证这套配置可以成功编译。

## Codex 版 pipeline 做了什么

这套流程主要在：

- `ObjectCounting/CodexProcess.h`
- `ObjectCounting/CodexProcess.cpp`
- `ObjectCounting/CodexPipeline_main.cpp`

它的思路比早期 HSV 路线复杂很多，大致是：

1. 从图像边缘估计背景颜色
2. 转到 Lab 空间，计算和背景之间的色度距离
3. 再叠加局部亮度残差和饱和度，生成一张前景分数图
4. 对分数图做 Otsu 二值化，得到初始 mask
5. 用闭运算、开运算、填洞、连通域过滤清理 mask
6. 统计连通域面积，对每个连通域单独估计应该分成几个目标
7. 通过腐蚀种子或距离峰值种子，在局部 mask 里做 seeded growth 拆分
8. 根据面积、填充率、轮廓等条件输出最终检测框和编号

这条线比原来的 `Process.cpp` 更“像一套完整系统”，因为它不仅给最终结果，还保存了中间产物，方便复盘。

## 这个分支留下了什么实验输出

输入样本当前在 `assets/input/`，包含：

- `coin1.jpg`
- `coin2.jpg`
- `pill1.jpg` 到 `pill7.jpg`

Codex 版批处理会把结果写到 `assets/output_codex/`。当前目录里有 54 张输出图，对应 9 张输入图各自的 6 类中间结果：

- `*_codex_score.png`：前景分数图
- `*_codex_raw_mask.png`：初始二值图
- `*_codex_clean_mask.png`：清理后的 mask
- `*_codex_split_mask.png`：拆分后的 mask
- `*_codex_markers.png`：局部标签 / markers 可视化
- `*_codex_result.png`：最终编号结果

更早一轮规则链实验输出保存在 `assets/output/`。

## 为什么这次实验还是失败了

这条分支已经比最初的 HSV counting 复杂很多，但问题并没有被真正解决。原因主要是：

- 背景估计依赖“图像边缘大多是背景”这个假设，一旦边缘有目标、阴影或杂物，前景分数图就会偏。
- 分数图本身仍然是手工组合的启发式，权重再复杂，本质上还是规则。
- Otsu、形态学清理、面积过滤这些步骤都很依赖样本分布，换一类图就要重新解释参数。
- 局部 seeded split 只能在已有 mask 基本正确时继续细分，前面的 mask 一旦错了，后面很难救回来。
- 药片和硬币已经能说明问题：目标类别、材质、边缘、反光和背景变化一大，共用规则就开始失效。

所以这次分支的失败不是“Codex 代码没写出来”，而是：

**Codex 把传统视觉方法推到了更远的地方，也更快证明了它的边界。**

## 这个分支的价值

虽然结果失败了，但这个分支仍然有价值：

- 它保留了一份很典型的纯 AI coding 样本。
- 它把“从 HSV 规则链到更复杂前景打分 pipeline”的演进完整留了下来。
- 它留下了大量中间结果图，不只是一个最终框图。
- 它比较清楚地暴露了传统视觉在多类型图像上的极限。

如果把它当成实验记录，这条分支是成功的；如果把它当成可复用方案，这条分支是失败的。

## 仓库结构

- `ObjectCounting/`：Visual Studio C++ 工程与主要实现
- `assets/input/`：输入样本
- `assets/output/`：较早规则链实验输出
- `assets/output_codex/`：Codex 版 pipeline 输出
- `docs/codex_pipeline.md`：Codex 版流程草稿说明
- `docs/notes.md`：这次分支的实验复盘

## 构建与运行

当前工程是 Visual Studio C++ 项目，已在本机环境下验证可以成功构建：

- 工具链：MSBuild / MSVC v145
- 语言标准：C++20
- OpenCV：链接本地 `D:\OpenCV\opencv\build`

工程里当前写死了本地依赖路径：

- Include: `D:\OpenCV\opencv\build\include`
- Lib: `D:\OpenCV\opencv\build\x64\vc16\lib`
- Dependency: `opencv_world4120d.lib`

当前 x64 默认入口是 `ObjectCounting/CodexPipeline_main.cpp`，会遍历：

- `D:/Projects/opencv-object-counting/assets/input`

并把结果输出到：

- `D:/Projects/opencv-object-counting/assets/output_codex`

## 如果以后继续

如果以后还继续这个方向，比继续叠规则更重要的是先承认边界：

- 先建立按类别拆分的最小评测集
- 先有真值和误差统计，不再只看单图观感
- 先确认哪些类别还能靠传统视觉，哪些已经不值得再硬推
- 如果目标真的是“多类型图像都要稳”，就该认真转向学习方法

这条 `exp/codex-test-fail` 分支最有价值的地方，不是它做成了什么，而是它把“纯 AI coding + 传统图像处理”这条路为什么会失败，留得很清楚。
