import SwiftUI
import AVFoundation
import UIKit

// MARK: - 主界面
struct ContentView: View {
    // 状态管理
    @State private var scannedCode: String?
    @State private var isShowingResult = false
    @State private var isFlashlightOn = false
    @State private var showScanFailureAlert = false
    @State private var lastRawScan: String = ""
    
    // 尺寸参数
    let scanSize: CGFloat = 280
    let cornerRadius: CGFloat = 26
    let borderLineWidth: CGFloat = 3
    
    var body: some View {
        ZStack {
            // 1. 底层：相机画面
            ColdVaultScannerView { code in
                // 只有当前没显示结果时才处理
                if !isShowingResult {
                    handleScanResult(code)
                }
            }
            .edgesIgnoringSafeArea(.all)
            
            // 2. 视觉层：遮罩 + 边框
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
                
                // C. 扫描线
                ModernScannerLine(width: scanSize - 20)
            }
            .edgesIgnoringSafeArea(.all)
            
            // 3. UI 交互层
            VStack {
                Text("Align QR Code within Frame")
                    .font(.system(.subheadline, design: .monospaced))
                    .foregroundColor(.white.opacity(0.8))
                    .padding(.top, 80)
                    .shadow(radius: 2)
                
                Spacer()
                
                // 手电筒按钮
                Button(action: toggleFlashlight) {
                    VStack(spacing: 8) {
                        Image(systemName: isFlashlightOn ? "flashlight.on.fill" : "flashlight.off.fill")
                            .font(.system(size: 24))
                        Text(isFlashlightOn ? "Light On" : "Light Off")
                            .font(.caption)
                    }
                    .foregroundColor(.white)
                    .frame(width: 80, height: 80)
                    .background(.ultraThinMaterial)
                    .clipShape(Circle())
                    .overlay(Circle().stroke(Color.white.opacity(0.2), lineWidth: 1))
                }
                .padding(.bottom, 60)
            }
        }
        // 结果弹窗
        .sheet(isPresented: $isShowingResult, onDismiss: {
            scannedCode = nil
        }) {
            if let code = scannedCode {
                ResultView(password: code)
                    .presentationDetents([.medium, .fraction(0.4)])
                    .presentationDragIndicator(.visible) // 这里已经开启了系统自带的灰色条
            }
        }
        .alert("Scan Failed", isPresented: $showScanFailureAlert) {
            Button("OK", role: .cancel) {}
        } message: {
            Text("The scanned code is not a valid ColdVault code. Please try again.")
        }
    }
    
    // 逻辑：处理扫描结果
    func handleScanResult(_ rawCode: String) {
        DispatchQueue.main.async {
            print("raw:", rawCode)
            let generator = UINotificationFeedbackGenerator()
            
            // 调用解密函数
            if let decrypted = decryptColdVaultCode(rawCode) {
                self.scannedCode = decrypted
                generator.notificationOccurred(.success)
                self.isShowingResult = true
            } else {
                generator.notificationOccurred(.warning)
                self.lastRawScan = rawCode
                self.showScanFailureAlert = true
                print("decrypt failed for:", rawCode)
            }
        }
    }
    
    // 逻辑：手电筒开关
    func toggleFlashlight() {
        guard let device = AVCaptureDevice.default(for: .video) else { return }
        if device.hasTorch {
            do {
                try device.lockForConfiguration()
                device.torchMode = isFlashlightOn ? .off : .on
                device.unlockForConfiguration()
                isFlashlightOn.toggle()
            } catch { }
        }
    }
}

// MARK: - 结果展示页 (修复重叠问题)
struct ResultView: View {
    let password: String
    @State private var isCopied = false
    
    var body: some View {
        VStack(spacing: 24) {
            Text("Decrypted Successfully")
                .font(.headline)
                .foregroundColor(.secondary)
                .padding(.top, 30) // 稍微增加一点顶部间距
            
            VStack(spacing: 12) {
                Text(password)
                    .font(.system(.title2, design: .monospaced))
                    .fontWeight(.semibold)
                    .foregroundColor(.primary)
                    .multilineTextAlignment(.center)
            }
            .padding(30)
            .frame(maxWidth: .infinity)
            .background(Color(.secondarySystemBackground))
            .cornerRadius(16)
            .padding(.horizontal)
            
            Button(action: {
                UIPasteboard.general.string = password
                let generator = UIImpactFeedbackGenerator(style: .medium)
                generator.impactOccurred()
                
                withAnimation { isCopied = true }
                DispatchQueue.main.asyncAfter(deadline: .now() + 2) { isCopied = false }
            }) {
                HStack {
                    Image(systemName: isCopied ? "checkmark" : "doc.on.doc")
                    Text(isCopied ? "Copied" : "Copy Password")
                }
                .fontWeight(.medium)
                .frame(maxWidth: .infinity)
                .padding()
                .background(isCopied ? Color.green : Color.blue)
                .foregroundColor(.white)
                .cornerRadius(12)
            }
            .padding(.horizontal)
            
            Spacer()
        }
        .background(Color(.systemBackground))
    }
}

// MARK: - 扫描线动画 (保持不变)
struct ModernScannerLine: View {
    let width: CGFloat
    @State private var isAnimating = false
    
    var body: some View {
        Rectangle()
            .fill(LinearGradient(gradient: Gradient(colors: [.clear, .cyan, .clear]), startPoint: .leading, endPoint: .trailing))
            .frame(width: width, height: 2)
            .shadow(color: .cyan, radius: 4)
            .offset(y: isAnimating ? 130 : -130)
            .onAppear {
                withAnimation(Animation.easeInOut(duration: 2.0).repeatForever(autoreverses: true)) {
                    isAnimating = true
                }
            }
    }
}

// MARK: - 底层功能函数 (保持不变)

// 1. 解密逻辑
func decryptColdVaultCode(_ code: String) -> String? {
    let key = "user0001"
    let prefix = "COLDVAULT:"
    
    guard code.hasPrefix(prefix) else { return nil }
    
    let hexString = String(code.dropFirst(prefix.count))
    var decryptedString = ""
    let keyChars = Array(key.utf8)
    var hexChars = Array(hexString)
    
    if hexChars.count % 2 != 0 { return nil }
    
    for i in stride(from: 0, to: hexChars.count, by: 2) {
        let hexPair = String(hexChars[i...i+1])
        if let byte = UInt8(hexPair, radix: 16) {
            let keyByte = keyChars[(i / 2) % keyChars.count]
            let decryptedByte = byte ^ keyByte
            decryptedString.append(Character(UnicodeScalar(decryptedByte)))
        }
    }
    return decryptedString
}

// 2. SwiftUI 摄像头包装器
struct ColdVaultScannerView: UIViewControllerRepresentable {
    var didScanCode: (String) -> Void
    
    func makeUIViewController(context: Context) -> ColdVaultCameraViewController {
        let controller = ColdVaultCameraViewController()
        controller.delegate = context.coordinator
        return controller
    }
    
    func updateUIViewController(_ uiViewController: ColdVaultCameraViewController, context: Context) {}
    
    func makeCoordinator() -> Coordinator {
        Coordinator(parent: self)
    }
    
    class Coordinator: NSObject, AVCaptureMetadataOutputObjectsDelegate {
        var parent: ColdVaultScannerView
        init(parent: ColdVaultScannerView) { self.parent = parent }
        
        func metadataOutput(_ output: AVCaptureMetadataOutput, didOutput metadataObjects: [AVMetadataObject], from connection: AVCaptureConnection) {
            if let metadataObject = metadataObjects.first {
                guard let readableObject = metadataObject as? AVMetadataMachineReadableCodeObject else { return }
                guard let stringValue = readableObject.stringValue else { return }
                parent.didScanCode(stringValue)
            }
        }
    }
}

// 3. UIKit 摄像头控制器
class ColdVaultCameraViewController: UIViewController {
    var captureSession: AVCaptureSession!
    var previewLayer: AVCaptureVideoPreviewLayer!
    weak var delegate: AVCaptureMetadataOutputObjectsDelegate?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = UIColor.black
        captureSession = AVCaptureSession()
        
        guard let videoCaptureDevice = AVCaptureDevice.default(for: .video) else { return }
        let videoInput: AVCaptureDeviceInput
        do { videoInput = try AVCaptureDeviceInput(device: videoCaptureDevice) } catch { return }
        
        if (captureSession.canAddInput(videoInput)) { captureSession.addInput(videoInput) } else { return }
        
        let metadataOutput = AVCaptureMetadataOutput()
        if (captureSession.canAddOutput(metadataOutput)) {
            captureSession.addOutput(metadataOutput)
            metadataOutput.setMetadataObjectsDelegate(delegate, queue: DispatchQueue.main)
            metadataOutput.metadataObjectTypes = [.qr]
        } else { return }
        
        previewLayer = AVCaptureVideoPreviewLayer(session: captureSession)
        previewLayer.frame = view.layer.bounds
        previewLayer.videoGravity = .resizeAspectFill
        view.layer.addSublayer(previewLayer)
        
        DispatchQueue.global(qos: .background).async { self.captureSession.startRunning() }
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        if let connection = self.previewLayer?.connection {
            let orientation = UIDevice.current.orientation
            if connection.isVideoOrientationSupported {
                switch (orientation) {
                case .portrait: connection.videoOrientation = .portrait
                case .landscapeRight: connection.videoOrientation = .landscapeLeft
                case .landscapeLeft: connection.videoOrientation = .landscapeRight
                case .portraitUpsideDown: connection.videoOrientation = .portraitUpsideDown
                default: connection.videoOrientation = .portrait
                }
            }
        }
        previewLayer?.frame = view.bounds
    }
}

