#include "GpuGrid3D.h"
#include <iostream>
#include <vector>

GLuint GpuGrid3D::create3DTexture(int internal_format, int format)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // alloc, fill nullptr
    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, m_width, m_height, m_depth,
        0, format, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
    return textureID;
}

GpuGrid3D::GpuGrid3D(int width, int height, int depth)
    : m_width(width), m_height(height), m_depth(depth)
{
    // density
    m_densityTexA = create3DTexture(GL_RGBA32F, GL_RGBA);
    m_densityTexB = create3DTexture(GL_RGBA32F, GL_RGBA);

    // velo
    m_velocityTexA = create3DTexture(GL_RGBA32F, GL_RGBA);
    m_velocityTexB = create3DTexture(GL_RGBA32F, GL_RGBA);

    // div - pressure
    m_divergenceTex = create3DTexture(GL_RGBA32F, GL_RGBA);
    m_pressureTexA = create3DTexture(GL_RGBA32F, GL_RGBA);
    m_pressureTexB = create3DTexture(GL_RGBA32F, GL_RGBA);
}

GpuGrid3D::~GpuGrid3D()
{
    GLuint textures[] = {
        m_densityTexA, m_densityTexB,
        m_velocityTexA, m_velocityTexB,
        m_divergenceTex, m_pressureTexA, m_pressureTexB
    };
    glDeleteTextures(7, textures);
}

void GpuGrid3D::clear(Shader& clearComputeShader)
{
    clearComputeShader.use();

    GLuint workGroupsX, workGroupsY, workGroupsZ;
    getWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    GLuint texturesToClear[] = {
        m_densityTexA, m_densityTexB,
        m_velocityTexA, m_velocityTexB,
        m_divergenceTex, m_pressureTexA, m_pressureTexB
    };

    for (GLuint tex : texturesToClear)
    {
        // bind tex to img unit0
        glBindImageTexture(0, tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        // launch 16x16x16 = 4096 work groups
        // 4096 * 512 = 2 097 152 threads (one for each 3D pixel)
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // finish write
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void GpuGrid3D::swapDensityBuffers()
{
    std::swap(m_densityTexA, m_densityTexB);
}

void GpuGrid3D::swapVelocityBuffers()
{
    std::swap(m_velocityTexA, m_velocityTexB);
}

void GpuGrid3D::step(Shader& splatShader, Shader& advectShader, Shader& diffuseShader,
    Shader& divergenceShader, Shader& pressureShader, Shader& gradientShader,
    const glm::vec3& mouse_pos3D, const glm::vec3& mouse_vel,
    bool is_bouncing, float dt,
    float viscosity, int diffuse_iterations, int pressure_iterations)
{
    splatShader.use();

    GLuint workGroupsX, workGroupsY, workGroupsZ;
    getWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    float brush_radius = m_width * 0.025f; // 2.5% of the obj

    // set const uniforms
    glUniform3fv(glGetUniformLocation(splatShader.ID, "u_brush_center3D"), 1, glm::value_ptr(mouse_pos3D));
    glUniform1f(glGetUniformLocation(splatShader.ID, "u_radius"), brush_radius);
    glUniform1i(glGetUniformLocation(splatShader.ID, "u_is_bouncing"), is_bouncing ? 1 : 0);

    // splat velo
    glUniform3fv(glGetUniformLocation(splatShader.ID, "u_force"), 1, glm::value_ptr(mouse_vel));

    // bind textures to img units - 0 (read) / 1 (write)
    glBindImageTexture(0, m_velocityTexA, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, m_velocityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // launch comp shader
    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // give it a moment

    swapVelocityBuffers(); // res->texA

    // splat dens
    glUniform3f(glGetUniformLocation(splatShader.ID, "u_force"), 1.0f, 0.0f, 0.0f);

    glBindImageTexture(0, m_densityTexA, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, m_densityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // *elevator music plays*

    swapDensityBuffers(); // res->texA

    // diffuse
    diffuseShader.use();

    // 6.0f
    float vel_a = dt * viscosity * m_width * m_width;
    float vel_rBeta = 1.0f / (1.0f + 6.0f * vel_a);

    float dens_a = dt * 0.00001f * m_width * m_width; // diff for smoke
    float dens_rBeta = 1.0f / (1.0f + 6.0f * dens_a);

    // diff velo
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_alpha"), vel_a);
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_rBeta"), vel_rBeta);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_b"), 1);

    for (int i = 0; i < diffuse_iterations; ++i)
    {
        if (i % 2 == 0)
        {
            // Read from A (u_x), Write to B (u_writeTexture)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
            glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);
            glBindImageTexture(2, m_velocityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        else
        {
            // Read from B (u_x), Write to A (u_writeTexture)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, m_velocityTexB);
            glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);
            glBindImageTexture(2, m_velocityTexA, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    // i=18 (even): Read A, Write B
    // i=19 (odd):  Read B, Write A.

    // diff velo
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_alpha"), dens_a);
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_rBeta"), dens_rBeta);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_densityTexA);
    glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_b"), 1);

    for (int i = 0; i < diffuse_iterations; ++i)
    {
        if (i % 2 == 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, m_densityTexA);
            glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);
            glBindImageTexture(2, m_densityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, m_densityTexB);
            glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);
            glBindImageTexture(2, m_densityTexA, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // divergence, pressure, gradient
    divergenceShader.use();
    glUniform3f(glGetUniformLocation(divergenceShader.ID, "u_gridSize"), (float)m_width, (float)m_height, (float)m_depth);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(divergenceShader.ID, "u_velocityField"), 0);

    glBindImageTexture(2, m_divergenceTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // pressure
    pressureShader.use();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_divergenceTex);
    glUniform1i(glGetUniformLocation(pressureShader.ID, "u_divergence"), 1);

    for (int i = 0; i < pressure_iterations; ++i)
    {
        if (i % 2 == 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, m_pressureTexA);
            glUniform1i(glGetUniformLocation(pressureShader.ID, "u_pressure"), 0);
            glBindImageTexture(2, m_pressureTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, m_pressureTexB);
            glUniform1i(glGetUniformLocation(pressureShader.ID, "u_pressure"), 0);
            glBindImageTexture(2, m_pressureTexA, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // gradient
    gradientShader.use();
    glUniform3f(glGetUniformLocation(gradientShader.ID, "u_gridSize"), (float)m_width, (float)m_height, (float)m_depth);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(gradientShader.ID, "u_velocityField"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_pressureTexA);
    glUniform1i(glGetUniformLocation(gradientShader.ID, "u_pressureField"), 1);

    glBindImageTexture(2, m_velocityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F); // Write to velocity B

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    swapVelocityBuffers();

    // advect
    advectShader.use();

    glUniform3f(glGetUniformLocation(advectShader.ID, "u_gridSize"), (float)m_width, (float)m_height, (float)m_depth);
    glUniform1f(glGetUniformLocation(advectShader.ID, "u_dt"), dt);

    // adv velo
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(advectShader.ID, "u_velocityField_sampler"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(advectShader.ID, "u_quantityToMove_sampler"), 1);

    glBindImageTexture(2, m_velocityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    swapVelocityBuffers(); // res->veloTexA

    // adv dens

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(advectShader.ID, "u_velocityField_sampler"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_densityTexA);
    glUniform1i(glGetUniformLocation(advectShader.ID, "u_quantityToMove_sampler"), 1);

    glBindImageTexture(2, m_densityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    swapDensityBuffers(); // res->densTexA





    // TODO: PROJ

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GpuGrid3D::getWorkGroups(GLuint& groupsX, GLuint& groupsY, GLuint& groupsZ)
{
    // total group = total size / group size
    groupsX = (m_width + 7) / 8;  // (128 + 7) / 8 ~ 16
    groupsY = (m_height + 7) / 8; // (128 + 7) / 8 ~ 16
    groupsZ = (m_depth + 7) / 8;  // (128 + 7) / 8 ~ 16
}