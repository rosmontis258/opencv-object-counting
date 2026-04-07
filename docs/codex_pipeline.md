# Codex Pipeline

这份流程是我单独写的一版传统图像处理计数方案，文件都带 `Codex` 前缀，方便和你原来的实现并排比较。

## 文件
- `ObjectCounting/CodexProcess.h`
- `ObjectCounting/CodexProcess.cpp`
- `ObjectCounting/CodexPipeline_main.cpp`

## 核心思路
1. 先从图像边缘估计背景颜色。
2. 在 Lab 空间计算前景与背景的色度距离。
3. 再叠加局部亮度残差和饱和度，生成一张前景响应图。
4. 对响应图做 Otsu 二值化。
5. 用形态学和连通域过滤清理噪声。
6. 对每个连通域单独做距离变换，生成局部峰值 marker。
7. 用 marker-based watershed 拆掉轻到中度粘连。
8. 最后按面积和填充率过滤，并输出编号结果图。

## 输出
运行后会把结果写到 `assets/output_codex/`：
- `*_codex_score.png`：前景响应图
- `*_codex_raw_mask.png`：初始二值图
- `*_codex_clean_mask.png`：清理后的 mask
- `*_codex_split_mask.png`：分水岭后的 mask
- `*_codex_markers.png`：marker / watershed 可视化
- `*_codex_result.png`：最终编号结果

## 说明
这版流程是为了更稳定地应对：
- 背景比较统一，但有阴影或高光
- 颜色对比不算特别强
- 目标之间存在轻到中度接触

它不是深度学习方案，也不是一套“一劳永逸”的万能规则，但更适合拿来学习完整的传统视觉处理链路。
