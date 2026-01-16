import SwiftUI

// 生成透明 UI 图片
struct TransparentUIExportView: View {
    // 复制 ContentView 里的参数
    let scanSize: CGFloat = 280
    let cornerRadius: CGFloat = 26
    let borderLineWidth: CGFloat = 3
    
    var body: some View {
        // 使用 ZStack，但不放 CameraView，底色设为 Clear
        ZStack {
            // 0. 极其重要：强制背景透明
            Color.clear.edgesIgnoringSafeArea(.all)
            
            // 1. 视觉层：遮罩 + 边框 (与主程序完全一致)
            ZStack {
                // A. 挖孔遮罩
                Color.black.opacity(0.6)
                    .mask(
                        ZStack {
                            Rectangle().fill(Color.white)
                            RoundedRectangle(cornerRadius: cornerRadius)
                                .fill(Color.black)
                                .frame(width: scanSize, height: scanSize)
                        }
                        .compositingGroup()
                        .luminanceToAlpha()
                    )
                
                // B. 白色边框
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(Color.white, lineWidth: borderLineWidth)
                    .frame(width: scanSize, height: scanSize)
                    .shadow(color: Color.white.opacity(0.6), radius: 8)
                
                // C. 扫描线 (这一帧是静止的，如果你想要动态很难，建议网页端用 CSS 做扫描线动画)
                // 这里我们画一个居中的扫描线作为示意，或者你可以注释掉它，在网页用 CSS 写动画
                Rectangle()
                    .fill(LinearGradient(gradient: Gradient(colors: [.clear, .cyan, .clear]), startPoint: .leading, endPoint: .trailing))
                    .frame(width: scanSize - 20, height: 2)
                    .shadow(color: .cyan, radius: 4)
            }
            .edgesIgnoringSafeArea(.all)
            
            // 2. UI 交互层
            VStack {
                Text("Align QR Code within Frame")
                    .font(.system(.subheadline, design: .monospaced))
                    .foregroundColor(.white.opacity(0.8))
                    .padding(.top, 80)
                    .shadow(radius: 2)
                
                Spacer()
                
                // 底部手电筒按钮 UI (仅展示，无功能)
                VStack(spacing: 8) {
                    Image(systemName: "flashlight.off.fill")
                        .font(.system(size: 24))
                    Text("Light Off")
                        .font(.caption)
                }
                .foregroundColor(.white)
                .frame(width: 80, height: 80)
                .background(.ultraThinMaterial)
                .clipShape(Circle())
                .overlay(Circle().stroke(Color.white.opacity(0.2), lineWidth: 1))
                .padding(.bottom, 60)
            }
        }
        .frame(width: 393, height: 852) // iPhone 15 Pro 的逻辑分辨率，确保比例对
    }
}

// 一个包裹视图，用来在模拟器上显示“导出按钮”
struct ExportWrapperView: View {
    var body: some View {
        VStack {
            Text("点击下方按钮导出透明 UI")
                .font(.headline)
            
            // 预览一下长什么样
            TransparentUIExportView()
                .frame(width: 200, height: 400) // 缩小预览
                .border(Color.red)
                .background(Color.checkerboard) // 棋盘格背景，方便看透明度
            
            // 导出按钮
            ShareLink(
                item: Image(uiImage: renderTransparentImage()),
                preview: SharePreview("UI Layer.png", image: Image(uiImage: renderTransparentImage()))
            ) {
                Label("导出透明 PNG", systemImage: "square.and.arrow.up")
                    .padding()
                    .background(Color.blue)
                    .foregroundColor(.white)
                    .cornerRadius(10)
            }
        }
    }
    
    // 核心渲染函数：把 View 转成透明 PNG
    @MainActor
    func renderTransparentImage() -> UIImage {
        let renderer = ImageRenderer(content: TransparentUIExportView())
        renderer.scale = 3.0 // 3x 分辨率 (iPhone Pro 标准)，保证高清
        renderer.isOpaque = false // ⚠️ 关键：允许透明背景
        
        return renderer.uiImage ?? UIImage()
    }
}

// 辅助：棋盘格背景，仅用于预览透明度
extension Color {
    static var checkerboard: Color {
        return Color.gray.opacity(0.2)
    }
}
