// gcc game_loop.c -o game_loop.exe -ld3d11 -luuid

#include <stdio.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d2d1.h>

// Variáveis globais DirectX
IDXGISwapChain *swapChain;
ID3D11Device *device;
ID3D11DeviceContext *deviceContext;
ID3D11RenderTargetView *backBuffer;

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
void InitFPSCounter(void);
void UpdateFPS(void);

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
        "Janela DirectX COM C",
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

    // Inicializando QueryPerformanceCounter
    InitFPSCounter();

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
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;     // Formato 32-bit cor
    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // Alvo de renderização
    scd.OutputWindow = hWnd;                                // Janela de saída
    scd.SampleDesc.Count = 1;                               // Anti-aliasing 4x
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;                                    // Modo janela

    const D3D_FEATURE_LEVEL level[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Criação do dispositivo, contexto e swap chain
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, level, 7,
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

// Renderiza um frame
void RenderFrame(void) {
    // Cor de limpeza do background (RGBA)
    float clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};

    // Limpeza do back buffer
    deviceContext->lpVtbl->ClearRenderTargetView(deviceContext, backBuffer, clearColor);

    // Apresentação do back buffer no front buffer
    swapChain->lpVtbl->Present(swapChain, 0, 0);
}

// Limpeza do DirectX
void CleanD3D(void) {
    swapChain->lpVtbl->SetFullscreenState(swapChain, FALSE, NULL); // Garantir que o modo fullscreen esteja desativado
    backBuffer->lpVtbl->Release(backBuffer);
    swapChain->lpVtbl->Release(swapChain);
    device->lpVtbl->Release(device);
    deviceContext->lpVtbl->Release(deviceContext);
}

void InitFPSCounter() {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startCounter);
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

        printf("FPS: %lf\n", fps);

        // Reseta os contadores
        frameCount = 0;
        elapsedTime = 0.0;
    }

    // Atualiza o contador de tempo inicial
    startCounter = endCounter;
}