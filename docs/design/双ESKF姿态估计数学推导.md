# SomatoSync 双 ESKF 姿态估计数学推导规范

**系统代号**：CoreMocap-DualESKF (V1.0 完全体)

---

## 1. 空间非正交斜交几何多样性布局代数证明

系统定义全节点板级主物理坐标系为 $\mathbf{B}$（基底包含相互垂直的 $\mathbf{x}_B, \mathbf{y}_B$）。物理世界作用在感知节点上的真实二维平面输入矢量（加速度或角速度）表示为：

$$\mathbf{f} = \begin{bmatrix} f_x \\ f_y \end{bmatrix}_B$$

依据正面 135° 与反面 90° 的非平行布局，建立两颗传感器坐标系到板级坐标系的标准仿射投影旋转矩阵：

$$\mathbf{R}_B^A = \mathbf{R}(135^\circ)^T = \begin{bmatrix} \cos 135^\circ & \sin 135^\circ \\ -\sin 135^\circ & \cos 135^\circ \end{bmatrix} = \begin{bmatrix} -\frac{\sqrt{2}}{2} & \frac{\sqrt{2}}{2} \\ -\frac{\sqrt{2}}{2} & -\frac{\sqrt{2}}{2} \end{bmatrix}$$

$$\mathbf{R}_B^B = \mathbf{R}(90^\circ)^T = \begin{bmatrix} \cos 90^\circ & \sin 90^\circ \\ -\sin 90^\circ & \cos 90^\circ \end{bmatrix} = \begin{bmatrix} 0 & 1 \\ -1 & 0 \end{bmatrix}$$

### 1.1 定理一：抗突发过载饱和的代数边界拓宽
设单颗 MEMS 传感器芯片的物理硬件量程极限为 $A_{\max}$。当患者产生剧烈动作，产生一个刚好处在 90° 方向：

$$\mathbf{f} = \begin{bmatrix} 0 \\ f_{\text{bump}} \end{bmatrix}_B \quad \text{其中 } f_{\text{bump}} > A_{\max}$$

此时，反面传感器 IMU_B 的二维输入由于投影关系直接表达为：

$$\mathbf{z}_B = \mathbf{R}_B^B \mathbf{f} = \begin{bmatrix} 0 & 1 \\ -1 & 0 \end{bmatrix} \begin{bmatrix} 0 \\ f_{\text{bump}} \end{bmatrix} = \begin{bmatrix} f_{\text{bump}} \\ 0 \end{bmatrix}_B$$

其 $x$ 轴测值直接吃满 $f_{\text{bump}}$。由于 $f_{\text{bump}} > A_{\max}$，反面 IMU_B 触发硬件级斩波饱和（Clipping），置信度失效。

然而此时，正面传感器 IMU_A 的二维吞入向量为：

$$\mathbf{z}_A = \mathbf{R}_B^A \mathbf{f} = \begin{bmatrix} -\frac{\sqrt{2}}{2} & \frac{\sqrt{2}}{2} \\ -\frac{\sqrt{2}}{2} & -\frac{\sqrt{2}}{2} \end{bmatrix} \begin{bmatrix} 0 \\ f_{\text{bump}} \end{bmatrix} = \begin{bmatrix} \frac{\sqrt{2}}{2}f_{\text{bump}} \\ -\frac{\sqrt{2}}{2}f_{\text{bump}} \end{bmatrix}_A$$

欲使正面 IMU_A 的分量轴也触发过载饱和，必须满足：

$$\frac{\sqrt{2}}{2}f_{\text{bump}} > A_{\max} \implies f_{\text{bump}} > \sqrt{2} A_{\max} \approx 1.414 A_{\max}$$

**结论**：在 45° 几何错位分流下，系统在最脆弱轴向上的动态抗过载边界被物理拓宽了 $\sqrt{2}$ 倍。当系统遭遇过载，状态机通过剔除饱和的 IMU_B，纯靠未饱和的 IMU_A 进行逆矩阵投影恢复（$\mathbf{f} = (\mathbf{R}_B^A)^{-1} \mathbf{z}_A$），保障了高动态运动下解算流不断流。

### 1.2 定理二：各向同性估计下白噪声噪底降低
定义两颗传感器的实际观测方程包含相互独立的传感器高斯白噪声：

$$\mathbf{z}_A = \mathbf{R}_B^A \mathbf{f} + \mathbf{n}_A, \quad \mathbf{z}_B = \mathbf{R}_B^B \mathbf{f} + \mathbf{n}_B \quad \text{其中 } \mathbf{n} \sim \mathcal{N}(\mathbf{0}, \sigma^2 \mathbf{I}_{2\times2})$$

将两路信号组合成一个全局观测矩阵：

$$\mathbf{Y}_{4\times1} = \mathbf{H}_{4\times2} \mathbf{f}_{2\times1} + \mathbf{N}_{4\times1} \implies \begin{bmatrix} \mathbf{z}_A \\ \mathbf{z}_B \end{bmatrix} = \begin{bmatrix} \mathbf{R}_B^A \\ \mathbf{R}_B^B \end{bmatrix} \mathbf{f} + \mathbf{N}$$

全局噪声协方差矩阵表示为 $\mathbf{R}_{\text{noise}} = \sigma^2 \mathbf{I}_{4\times4}$。根据高斯-马尔可夫最优估计（Gauss-Markov Theorem）理论，板级最终融合估计状态的后验协方差矩阵 $\mathbf{P}$ 为观测系统信息矩阵的逆：

$$\mathbf{P} = \left( \mathbf{H}^T \mathbf{R}_{\text{noise}}^{-1} \mathbf{H} \right)^{-1} = \sigma^2 \left( \mathbf{H}^T \mathbf{H} \right)^{-1}$$

代入子空间投影矩阵：

$$\mathbf{H}^T \mathbf{H} = \begin{bmatrix} (\mathbf{R}_B^A)^T & (\mathbf{R}_B^B)^T \end{bmatrix} \begin{bmatrix} \mathbf{R}_B^A \\ \mathbf{R}_B^B \end{bmatrix} = (\mathbf{R}_B^A)^T \mathbf{R}_B^A + (\mathbf{R}_B^B)^T \mathbf{R}_B^B$$

由于 $\mathbf{R}_B^A$ 与 $\mathbf{R}_B^B$ 属于标准的二维特殊正交群 $SO(2)$，满足 $(\mathbf{R}_B^A)^T \mathbf{R}_B^A = \mathbf{I}_{2\times2}$ 且 $(\mathbf{R}_B^B)^T \mathbf{R}_B^B = \mathbf{I}_{2\times2}$。
代入化简：

$$\mathbf{H}^T \mathbf{H} = \mathbf{I}_{2\times2} + \mathbf{I}_{2\times2} = 2 \mathbf{I}_{2\times2}$$

$$\mathbf{P} = \sigma^2 (2 \mathbf{I}_{2\times2})^{-1} = \begin{bmatrix} \frac{\sigma^2}{2} & 0 \\ 0 & \frac{\sigma^2}{2} \end{bmatrix}$$

计算噪底标准差：

$$\sigma_{\text{board}} = \sqrt{\frac{\sigma^2}{2}} = \frac{\sigma}{\sqrt{2}} \approx 0.707 \sigma$$

**结论**：估计状态在板级坐标系上的后验方差对称降低为 $\frac{\sigma^2}{2}$。证明了 45° 斜交多样性布局实现了全向一致的 $\sqrt{2}$ 倍物理噪底净化（噪声标准差下降至 $70.7\%$）。

---

## 2. 误差状态卡尔曼滤波 (ESKF) 状态估计公式

> **工程选型说明**：对于本项目的穿戴式人体关节追踪场景，设备刚性绑定在骨骼段上，仅需测量各骨骼段之间的相对旋转，不需要也不可能仅凭 IMU 测出关节在房间里的绝对坐标。因此系统采用**高动态优化的 6 维 ESKF**（姿态误差 3 + 陀螺仪零偏 3），而非理论通用的 15 维 ESKF（含位置、速度、重力、加速度计零偏等）。在没有外部定位锚点（如 GPS、UWB、视觉）的情况下，位置、速度等状态是不可观测的，引入它们不仅白白浪费 MCU 算力，还会因可观性不足导致算法数值发散。6 维 ESKF 在工程上是最稳定、最高效、最不易发散的选择。

ESKF 的核心设计思想为：**真值状态 = 名义状态 $\oplus$ 误差状态**。名义状态在高频进行非线性动力学积分，误差状态在卡尔曼滤波器中低频进行雅可比线性递推。

### 2.1 状态向量全要素定义
系统构建的 6 维误差状态方程如下：

- **名义状态向量**（7 维，四元数 + 陀螺零偏）：
  $$\mathbf{x} = \begin{bmatrix} \mathbf{q} & \mathbf{w}_b \end{bmatrix}^T$$
  其中，$\mathbf{q}$ 为单位四元数（4D），$\mathbf{w}_b$ 为陀螺仪零偏（3D）。

- **误差状态向量**（6 维，李代数局部切空间）：
  $$\delta\mathbf{x} = \begin{bmatrix} \delta\boldsymbol{\theta} & \delta\mathbf{w}_b \end{bmatrix}^T \in \mathbb{R}^{6}$$
  其中，$\delta\boldsymbol{\theta}$ 为李群 $SO(3)$ 局部误差对应的李代数向量 $\in \mathbb{R}^3$。

### 2.2 误差状态系统时间传播方程
连续时间下，误差状态的一阶微分线性微分方程建模为：

$$\delta\dot{\mathbf{x}} = \mathbf{F}_c \delta\mathbf{x} + \mathbf{G}_c \mathbf{w}$$

代数展开对应如下雅可比传递矩阵：

$$\mathbf{F}_c = \begin{bmatrix} 
-[\boldsymbol{\omega}_m - \mathbf{w}_b]_\times & -\mathbf{I}_{3\times3} \\
\mathbf{0}_{3\times3} & \mathbf{0}_{3\times3}
\end{bmatrix}$$

其中，$\boldsymbol{\omega}_m$ 为双源并发融合后输出的空间角速度测量值。

### 2.3 协方差离散时间传播 (Predict Step)
在 1000Hz 采样节拍下（采样间隔 $\Delta t$），进行状态协方差矩阵 $\mathbf{P}$ 的时间传播：

$$\mathbf{F} \approx \mathbf{I}_{6\times6} + \mathbf{F}_c \Delta t$$

$$\mathbf{P}_{k|k-1} = \mathbf{F} \mathbf{P}_{k-1|k-1} \mathbf{F}^T + \mathbf{Q}$$

其中，$\mathbf{Q}$ 为离散时间系统过程噪声协方差矩阵。

### 2.4 重力矢量约束更新 (Update Step)
当运动趋于缓和，处于相对静止或者匀速直线运动时，加速度量测包含极强的重力信息。系统以加速度测量值作为观测约束，纠正累积的航向漂移：

- **量测残差 (Innovation)**：
  $$\mathbf{r} = \mathbf{a}_m - \mathbf{R}(\mathbf{q})^T \mathbf{g}$$
- **观测雅可比矩阵 $\mathbf{H}$** (大小为 $3 \times 6$)：
  $$\mathbf{H} = \begin{bmatrix} [\mathbf{R}(\mathbf{q})^T \mathbf{g}]_\times & \mathbf{0}_{3\times3} \end{bmatrix}$$
- **卡尔曼增益求解**：
  $$\mathbf{K} = \mathbf{P}_{k|k-1} \mathbf{H}^T (\mathbf{H} \mathbf{P}_{k|k-1} \mathbf{H}^T + \mathbf{V})^{-1}$$
- **误差状态后验更新**：
  $$\delta\mathbf{x} = \mathbf{K} \mathbf{r}$$
- **名义状态反馈注入 (Feedback & Injection)**：
  $$\mathbf{q}_{\text{post}} = \mathbf{q}_{\text{prior}} \otimes \text{Exp}(\delta\boldsymbol{\theta})$$
  $$\mathbf{w}_{b,\text{post}} = \mathbf{w}_{b,\text{prior}} + \delta\mathbf{w}_b$$
- **后验协方差更新（协方差对称化）**：
  系统采用解析稀疏展开的协方差更新，配合强制对称化 $P_{ij} = P_{ji} = (P_{ij} + P_{ji}) / 2$ 保证正定性，无需完整的 Joseph 形式以节省算力。
