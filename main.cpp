#include "imgui/imgui.h"
#include "include/glfw3.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>
// 窗口尺寸
const int WINDOW_WIDTH = 2400;
const int WINDOW_HEIGHT = 1600;

// 网格尺寸（每个格子的大小）
const int GRID_SIZE = 20; // 每个格子的大小为 20 像素
// 存储点的位置和颜色
struct GridPoint
{
    ImVec2 position; // 点的位置（基于坐标系）
    ImU32 color;     // 点的颜色
};
std::vector<GridPoint> points;                // 存储所有标记的点
std::vector<std::pair<ImVec2, ImVec2>> lines; // 存储所有连线

// 标记点
void MarkPoint(const ImVec2 &point, ImU32 color)
{
    points.push_back({point, color});
}

// 画网格和坐标系
void DrawGrid(float zoom_factor, const ImVec2 &offset)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();                                     // 获取画布的起始位置
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + WINDOW_WIDTH, canvas_p0.y + WINDOW_HEIGHT); // 画布的结束位置

    // 绘制网格线
    for (int i = 0; i <= WINDOW_WIDTH / GRID_SIZE; ++i)
    {
        float x = canvas_p0.x + i * (GRID_SIZE * zoom_factor) + offset.x;
        draw_list->AddLine(ImVec2(x, canvas_p0.y + offset.y),
                           ImVec2(x, canvas_p1.y + offset.y),
                           IM_COL32(200, 200, 200, 255)); // 垂直线
    }
    for (int i = 0; i <= WINDOW_HEIGHT / GRID_SIZE; ++i)
    {
        float y = canvas_p0.y + i * (GRID_SIZE * zoom_factor) + offset.y;
        draw_list->AddLine(ImVec2(canvas_p0.x + offset.x, y),
                           ImVec2(canvas_p1.x + offset.x, y),
                           IM_COL32(200, 200, 200, 255)); // 水平线
    }

    // 绘制标记的点
    for (const auto &point : points)
    {
        float x = canvas_p0.x + point.position.x * (GRID_SIZE * zoom_factor) + offset.x;
        float y = canvas_p0.y + point.position.y * (GRID_SIZE * zoom_factor) + offset.y;
        draw_list->AddCircleFilled(ImVec2(x, y), 5.0f, point.color); // 画点

        // 检查鼠标是否悬停在标记点上
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float distance = std::sqrt((mouse_pos.x - x) * (mouse_pos.x - x) + (mouse_pos.y - y) * (mouse_pos.y - y));
        if (distance < 10.0f)
        { // 如果鼠标距离点小于 10 像素
            // 显示提示框
            ImGui::BeginTooltip();
            ImGui::Text("Point: (%d, %d)", static_cast<int>(point.position.x), static_cast<int>(point.position.y));
            if (point.color == IM_COL32(0, 255, 255, 255))
            { // Aqua 色代表服务器
                ImGui::Text("Type: Server");
            }
            else if (point.color == IM_COL32(255, 0, 0, 255))
            { // 红色代表客户端
                ImGui::Text("Type: Client");
            }
            ImGui::EndTooltip();
        }
    }

    // 绘制连线
    for (const auto &line : lines)
    {
        float x1 = canvas_p0.x + line.first.x * (GRID_SIZE * zoom_factor) + offset.x;
        float y1 = canvas_p0.y + line.first.y * (GRID_SIZE * zoom_factor) + offset.y;
        float x2 = canvas_p0.x + line.second.x * (GRID_SIZE * zoom_factor) + offset.x;
        float y2 = canvas_p0.y + line.second.y * (GRID_SIZE * zoom_factor) + offset.y;
        draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(255, 165, 0, 255), 2.0f); // 画线（Orange 色）
    }
}

int main()
{
    // 初始化GLFW
    if (!glfwInit())
        return -1;

    // 创建窗口
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ImGui Grid", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 开启垂直同步

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 主循环
    float zoom_factor = 1.0f;         // 初始缩放因子
    ImVec2 offset = {0.0f, 0.0f};     // 初始偏移量
    ImVec2 selected_point = {-1, -1}; // 记录选中的点
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 创建窗口并设置大小
        ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT)); // 设置窗口大小
        ImGui::Begin("Grid Window", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        // 处理鼠标滚轮缩放
        if (ImGui::IsWindowHovered())
        {
            float zoom_delta = io.MouseWheel * 0.1f;
            zoom_factor += zoom_delta;
            zoom_factor = std::clamp(zoom_factor, 0.5f, 5.0f); // 限制缩放范围
        }

        // 处理鼠标拖动平移（Alt + 鼠标移动）
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(0) && io.KeyAlt)
        {
            offset.x += io.MouseDelta.x;
            offset.y += io.MouseDelta.y;
        }

        // 绘制网格和坐标系
        DrawGrid(zoom_factor, offset);

        // 处理鼠标点击（仅在未按住 Alt 键时标记点）
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !io.KeyAlt)
        {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();

            // 计算精确的网格坐标（浮点数）
            float exact_grid_x = (mouse_pos.x - canvas_p0.x - offset.x) / (GRID_SIZE * zoom_factor);
            float exact_grid_y = (mouse_pos.y - canvas_p0.y - offset.y) / (GRID_SIZE * zoom_factor);

            // 四舍五入到最近的整数网格坐标
            int grid_x = static_cast<int>(std::round(exact_grid_x));
            int grid_y = static_cast<int>(std::round(exact_grid_y));

            // 计算与网格点的偏移距离
            float dx = exact_grid_x - grid_x;
            float dy = exact_grid_y - grid_y;
            const float snap_radius = 0.2f; // 吸附半径（网格单位）

            // 检查是否在有效范围内且靠近网格点
            if (grid_x >= 0 && grid_x < WINDOW_WIDTH / GRID_SIZE &&
                grid_y >= 0 && grid_y < WINDOW_HEIGHT / GRID_SIZE &&
                dx * dx + dy * dy <= snap_radius * snap_radius)
            {

                // 如果按住 Shift 键，则进行连线操作
                if (io.KeyShift)
                {
                    if (selected_point.x == -1 && selected_point.y == -1)
                    {
                        selected_point = ImVec2(grid_x, grid_y); // 选中第一个点
                    }
                    else
                    {
                        // 连接两个点
                        lines.push_back({selected_point, ImVec2(grid_x, grid_y)});
                        selected_point = ImVec2(-1, -1); // 重置选中的点
                    }
                }
                else
                {
                    // 标记点（Aqua 色代表服务器，红色代表客户端）
                    if (io.KeyCtrl)
                    {
                        MarkPoint(ImVec2(grid_x, grid_y), IM_COL32(255, 0, 0, 255));
                    }
                    else
                    {
                        MarkPoint(ImVec2(grid_x, grid_y), IM_COL32(0, 255, 255, 255));
                    }
                }
            }
        }

        ImGui::End();

        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f); // 设置背景颜色
        glClear(GL_COLOR_BUFFER_BIT);             // 清除背景
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}