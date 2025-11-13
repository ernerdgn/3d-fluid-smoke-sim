# Real-Time 2D & 3D Fluid/Smoke Simulator

![C++](https://img.shields.io/badge/C++-blue.svg)
![OpenGL](https://img.shields.io/badge/OpenGL-4.3+-brightgreen.svg)
![GLSL](https://img.shields.io/badge/GLSL-Compute_Shaders-orange.svg)
![GLM](https://img.shields.io/badge/lib-GLM-blueviolet.svg)

This project is a high-performance fluid and smoke simulator built from scratch in C++ and modern OpenGL. It began as a 2D CPU prototype and evolved into a fully 3D, GPU-accelerated simulation rendered in real-time using a volumetric ray marcher.

---

## Preview

### 3D Real-Time Simulation (GPU Compute)
*The final 3D simulation, running at 64x64x64 resolution in real-time. Physics are executed with OpenGL Compute Shaders, and the volume is rendered with a ray marcher.*

![](https://i.imgur.com/cdYBtmo.gif)

### 2D Simulation (CPU Prototype)
*The initial 2D prototype, which proved the physics logic. This version runs on the CPU and visualizes the data by uploading it to an OpenGL texture.*

![](https://i.imgur.com/fxqvjPV.gif)

---

## Features

* **Real-Time 3D Simulation:** A `64x64x64` (or higher) grid is simulated and rendered at interactive framerates.
* **GPU-Accelerated Physics:** All physics calculations (`advect`, `diffuse`, `project`) are ported from the CPU to high-performance OpenGL Compute Shaders.
* **Volumetric Ray Marching:** The 3D density grid is rendered as a semi-transparent volume using a custom GLSL ray marching fragment shader.
* **3D Mouse Interaction:** A 3D brush "paints" density and velocity into the volume by ray-casting 2D mouse coordinates into the 3D simulation space.
* **Full 3D Camera:** The simulation cube can be rotated and inspected from any angle.
* **Debug Views:** Switch between rendering Density, Velocity, and Pressure fields in real-time to debug the simulation.

---

## Physics

This simulator solves a simplified version of the **Navier-Stokes equations** for incompressible fluid flow. The core physics model is based on the "Stable Fluids" paper by Jos Stam, which breaks the complex physics into a sequence of smaller, solvable steps.

Each frame, the simulation executes this pipeline:

1.  **Splat (Forces):** Add new forces.
2.  **Diffuse (Viscosity):** Spread the fluid out.
3.  **Project (Incompressibility):** Make the fluid "swirl" and not "pile up."
4.  **Advect (Movement):** Move the fluid along its own velocity field.

Here is the math behind each step:

### 1. Advection
This step moves the fluid's properties (like density or velocity) along the existing velocity field.

* **Method:** We use a **Semi-Lagrangian** or "back-tracing" method. For each cell in the *new* grid, we trace its position backward in time along the velocity field to find where it *came from* in the *old* grid.
* **Math:** We find the previous position $P_{prev} = P_{current} - \vec{v}(P_{current}) \cdot \Delta t$. We then sample the old grid at $P_{prev}$ (using trilinear interpolation for 3D) and write that value to $P_{current}$ in the new grid. This is unconditionally stable and prevents the simulation from "blowing up."

### 2. Diffusion (Viscosity)
This step simulates the fluid's "thickness," or viscosity. It's a solver step that makes the fluid's properties "spread out" to their neighbors.

* **Math:** This step solves a form of the **Poisson equation**: $\frac{\partial u}{\partial t} = \nu \nabla^2 u$, where $\nu$ is the viscosity. We are solving for the new state $u$ that satisfies $u = u_0 + \nu \nabla^2 u \cdot \Delta t$.
* This rewrites to a linear system: $(I - \nu \Delta t \nabla^2)u = u_0$.

> **Solver Iterations (Gauss-Seidel vs. Jacobi)**
>
> * **Gauss-Seidel (CPU):** In the 2D CPU prototype, this was solved with a **Gauss-Seidel** solver. This iterative method updates each cell to the average of its neighbors, *using the brand-new values* of neighbors that have already been updated in the same loop. This is fast on a single-threaded CPU but is impossible to run in parallel on a GPU (it creates a race condition).
>
> * **Jacobi (GPU):** For the 3D GPU port, we switched to a **Jacobi iteration**. This method is parallel-friendly. In each iteration, every cell is updated to the average of its neighbors *using only the data from the previous iteration*. This means all 262,000+ texels can be calculated in parallel. We run this solver 4-20 times per frame to get a good result.

### 3. Projection (Incompressibility)
This is the most important step. It makes the fluid incompressible, meaning it can't be "piled up" or "destroyed." This is what creates all the realistic swirls.

* **Math:** This step enforces the *incompressibility condition*, which states that the **divergence** of the velocity field must be zero ($\nabla \cdot \vec{v} = 0$).
* **The Process:**
    1.  **Calculate Divergence:** First, we run a compute shader (`divergence.comp`) to calculate the divergence ($\nabla \cdot \vec{v}$) at every texel. This tells us *where* the fluid is "piling up" (positive divergence) or "spreading out" (negative divergence).
    2.  **Solve for Pressure:** We must find a "pressure" field `p` that, when subtracted, will "fix" this divergence. This requires solving another **Poisson equation**: $\nabla^2 p = \nabla \cdot \vec{v}$. We solve this using our iterative Jacobi solver (`pressure.comp`).
    3.  **Subtract Gradient:** Finally, we run a shader (`gradient.comp`) that subtracts the *gradient* (the "slope") of the pressure field from our velocity field: $\vec{v}_{new} = \vec{v}_{old} - \nabla p$. This new velocity field is now divergence-free (incompressible).

---

## 2D CPU to 3D GPU

This project was built in several distinct stages, which was key to managing the complexity.

### Part 1: The 2D CPU Prototype
The entire simulation was first built in plain C++, with the fluid grids (`density`, `velocity`) stored in `std::vector`.
* **Goal:** To prove the physics logic of the `advect`, `diffuse`, and `project` steps was correct.
* **Rendering:** The `density` vector was uploaded to a 2D OpenGL texture (`glTexSubImage2D`) every frame and drawn to a quad.
* **Result:** The simulation worked, but was **extremely slow** (1-2 FPS), especially at `128x128` resolution. The iterative solvers (`diffuse`, `project`) were the bottleneck.

### Part 2: The 2D GPU Simulation (FBOs)
The 2D simulation was then ported to the GPU.
* **Grids:** `std::vector` was replaced with `GL_TEXTURE_2D`.
* **Physics:** The C++ functions were rewritten as GLSL fragment shaders (`.frag`).
* **Method:** We used a "ping-pong" technique, writing results to a **Frame Buffer Object (FBO)** attached to `textureB` while reading from `textureA`, and then swapping.
* **Result:** A massive speedup, running at 60+ FPS on a `512x512` grid.

### Part 3: The 3D GPU Simulation (Compute Shaders)
Moving to 3D represented an exponential leap in data.
* **The Problem:** A `128x128` grid is 16,384 cells. A `128x128x128` grid is **2,097,152** cells.
* **The "Slow" 3D Way:** The FBO "slice-by-slice" method (running a 2D fragment shader 128 times) was too slow, creating thousands of draw calls per frame.
* **The "Fast" 3D Way:** We refactored the *entire* backend to use **Compute Shaders**.
    * **Grids:** `GL_TEXTURE_3D` and `image3D` for direct read/write access.
    * **Physics:** All shaders were rewritten as `.comp` files.
    * **Method:** Instead of 128 draw calls *per iteration*, we now dispatch *one* `glDispatchCompute` command. This reduced our `diffuse` step from `20 * 128 = 2,560` draw calls to just `20` dispatch calls, enabling real-time performance.

### Part 4: 3D Volumetric Rendering (Ray Marching)
We needed a way to *see* the 3D grid.
* **Method:** A ray marching fragment shader (`raymarch.frag`) is run on the 6 faces of a 3D cube.
* **Logic:** For each pixel, the shader marches a ray *through* the 3D density texture. It samples the density at each step and uses **back-to-front** blending to accumulate color and alpha, resulting in a correct, semi-transparent volumetric cloud.

### Part 5: 3D Interaction (Ray Casting)
The final step was to poke the 3D fluid(seems much more like a smoke).
* **Method:** We use the camera's inverse `projection * view` matrices to "un-project" the 2D mouse coordinates into a 3D world-space ray.
* **Logic:** We find where this ray intersects an invisible plane at `z=0`. We then use the cube's `inverse(model)` matrix to transform this 3D *world* point into the cube's *local* 3D grid coordinate. This coordinate is sent to the `splat.comp` shader as the 3D brush center.

---

## Building & Running

### Dependencies
* [**GLAD**](https://glad.dav1d.de/) (OpenGL Loader)
* [**GLFW**](https://www.glfw.org/) (Windowing & Input)
* [**GLM**](https://github.com/g-truc/glm) (Math Library)

### Build
This project uses a standard CMake build:
```bash
# Clone the repo
git clone [your-repo-url]
cd [your-repo-name]

# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .
```

---

## Controls
- LMB: Splat (add) density and velocity.

- RMB (Hold): Rotate the 3D camera.

- 'D' Key: Show Density (Rendered Volume).

- 'V' Key: Show Velocity (Debug 2D).

- 'P' Key: Show Pressure (Debug 2D).

---

## TODO
- 3D Vorticity Confinement: Implement the 3D version of curl to add back small-scale, turbulent details.

- Enable other display modes for 3D debug.