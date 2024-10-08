// gcc game_loop.c -o game_loop.exe -ld3d11 -ld2d1 -ldxgi -ldwrite -luuid

#include <stdio.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <initguid.h>

#ifdef DEFINE_GUID

DEFINE_GUID(IID_IDWriteFactory, 
0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb,0x48);

#endif  // DEFINE_GUID

// Variáveis globais DirectX
IDXGISwapChain *swapChain;
ID3D11Device *device;
ID3D11DeviceContext *deviceContext;
ID3D11RenderTargetView *backBuffer;

ID2D1RenderTarget *renderTarget;
ID2D1SolidColorBrush *brush;

IDWriteFactory *writeFactory;
IDWriteTextFormat *textFormat;

// Variáveis para FPS
LARGE_INTEGER frequency, startCounter, endCounter;
double frameTime = 0.0;
double fps = 0.0;
int frameCount = 0;
double elapsedTime = 0.0;

// Prototipação
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitD3D(HWND hWnd);
void InitD2D(HWND hWnd);
void CleanD3D(void);
void RenderFrame(void);
void UpdateFPS(void);
void InitFPSCounter(void);
void InitTextRender(void);
void RenderFPS();

// Função principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Criação da janela
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WindowClass";

    RegisterClassEx(&wc);

    RECT wr = {0, 0, 800, 600};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowEx(0,
        "WindowClass",
        "Janela DirectX",
        WS_OVERLAPPEDWINDOW,
        300, 300,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hWnd, nCmdShow);

    // Inicialização do DirectX
    InitD3D(hWnd);
    InitD2D(hWnd);

    // Inicializando QueryPerformanceCounter
    InitFPSCounter();
    InitTextRender();

    // Loop principal
    MSG msg = {0};
    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Renderização de cada frame
        RenderFrame();

        // Calcular e exibir FPS
        UpdateFPS();
        //RenderFPS();
    }

    // Limpeza do DirectX
    CleanD3D();

    return msg.wParam;
}

// Processamento de mensagens da janela
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// Inicializa o Direct3D
void InitD3D(HWND hWnd) {
    // Configuração da swap chain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;                                    // Um back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // Formato 32-bit cor
    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // Alvo de renderização
    scd.OutputWindow = hWnd;                                // Janela de saída
    scd.SampleDesc.Count = 4;                               // Anti-aliasing 4x
    scd.Windowed = TRUE;                                    // Modo janela

    // Criação do dispositivo, contexto e swap chain
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &scd, &swapChain, &device, NULL, &deviceContext);

    // Criação do back buffer
    ID3D11Texture2D *pBackBuffer;
    swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)pBackBuffer, NULL, &backBuffer);
    pBackBuffer->lpVtbl->Release(pBackBuffer);

    // Configurando o alvo de renderização
    deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1, &backBuffer, NULL);

    // Definindo o viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 800;
    viewport.Height = 600;
    deviceContext->lpVtbl->RSSetViewports(deviceContext, 1, &viewport);
}

void InitD2D(HWND hWnd) {
    D2D1_COLOR_F colorBack = {0.0f, 0.0f, 0.0f, 1.0f};
    D2D1_COLOR_F colorWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    D2D1_PIXEL_FORMAT pixelFormat = {
            DXGI_FORMAT_R8G8B8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        };
    D2D1_RENDER_TARGET_PROPERTIES props = {
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        pixelFormat,
        0,
        0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    };

    // Inicializar Direct2D
    ID2D1Factory *d2dFactory;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void**)&d2dFactory);

    // Configurar as propriedades do render target
    IDXGISurface *dxgiBackBuffer;
    swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_IDXGISurface, (void**)&dxgiBackBuffer);

    HRESULT hr = d2dFactory->lpVtbl->CreateDxgiSurfaceRenderTarget(d2dFactory, dxgiBackBuffer, &props, &renderTarget);

    if (FAILED(hr)) {
        printf("Falha ao criar o renderTarget. HRESULT: 0x%08X\n", hr);
        return; // Evite continuar se o renderTarget falhar
    }


    dxgiBackBuffer->lpVtbl->Release(dxgiBackBuffer);

    // Criar o brush
    ((ID2D1RenderTarget*)renderTarget)->lpVtbl->CreateSolidColorBrush((ID2D1RenderTarget*)renderTarget, &colorWhite, NULL, &brush);
}

// Renderiza um frame
void RenderFrame(void) {
    // Cor de limpeza do background (RGBA)
    float clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};

    // Limpeza do back buffer
    deviceContext->lpVtbl->ClearRenderTargetView(deviceContext, backBuffer, clearColor);

    // Apresentação do back buffer no front buffer
    swapChain->lpVtbl->Present(swapChain, 0, 0);
}

// Calcula e exibe o FPS
void UpdateFPS(void) {
    // Obtém o tempo atual
    QueryPerformanceCounter(&endCounter);

    // Calcula o tempo decorrido em segundos
    frameTime = (double)(endCounter.QuadPart - startCounter.QuadPart) / frequency.QuadPart;

    // Atualiza o contador de frames e o tempo acumulado
    frameCount++;
    elapsedTime += frameTime;

    // Atualiza o FPS a cada segundo (ou seja, após acumular 1 segundo)
    if (elapsedTime >= 1.0) {
        fps = frameCount / elapsedTime;

        // Reseta os contadores
        frameCount = 0;
        elapsedTime = 0.0;
    }

    // Atualiza o contador de tempo inicial
    startCounter = endCounter;
}

void InitFPSCounter() {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startCounter);
}

void InitTextRender(void) {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&writeFactory);
    
    writeFactory->lpVtbl->CreateTextFormat(writeFactory, L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                           DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"en-us", &textFormat);
}

void RenderFPS() {
    D2D1_RECT_F layoutRect = {650.0f, 10.0f, 800.0f, 50.0f};
    wchar_t fpsText[16];
    swprintf(fpsText, 16, L"FPS: %.2f", fps);

    // Começar a desenhar com Direct2D
    ((ID2D1RenderTarget*)renderTarget)->lpVtbl->BeginDraw((ID2D1RenderTarget*)renderTarget);

    // Desenhar o texto
    // Desenha o FPS Clock no canto superior direito
    ((ID2D1RenderTarget*)renderTarget)->lpVtbl->DrawText(
        (ID2D1RenderTarget*)renderTarget,
        fpsText,
        wcslen(fpsText),
        textFormat,
        &layoutRect,
        (ID2D1Brush*)brush,
        D2D1_DRAW_TEXT_OPTIONS_NONE,
        DWRITE_MEASURING_MODE_NATURAL
    );

    // Finalizar o desenho
    ((ID2D1RenderTarget*)renderTarget)->lpVtbl->EndDraw((ID2D1RenderTarget*)renderTarget, NULL, NULL);
}

// Limpeza do DirectX
void CleanD3D(void) {
    swapChain->lpVtbl->SetFullscreenState(swapChain, FALSE, NULL); // Garantir que o modo fullscreen esteja desativado
    backBuffer->lpVtbl->Release(backBuffer);
    swapChain->lpVtbl->Release(swapChain);
    device->lpVtbl->Release(device);
    deviceContext->lpVtbl->Release(deviceContext);
}
