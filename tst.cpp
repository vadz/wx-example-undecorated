// This is a simple example of a wxWidgets application using window without the
// standard "chrome", i.e. title bar and borders.
//
// With MSVS you can use the provided solution file to build it. With other
// compilers, remmeber to link against dwmapi.lib in addition to all the rest.
#include "wx/app.h"
#include "wx/button.h"
#include "wx/frame.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/stattext.h"

#ifdef __WXMSW__
    #include "wx/msw/wrapwin.h"

    #include <dwmapi.h>
    #include <windowsx.h>

    #include "wx/msw/private.h" // for wxGetSystemMetrics()
    #include "wx/msw/winundef.h"

#ifdef _MSC_VER
    #pragma comment(lib, "dwmapi.lib")
#endif
#endif // __WXMSW__

class MyPanel : public wxPanel
{
public:
    explicit MyPanel(wxWindow* parent) : wxPanel(parent) {
    }

#ifdef __WXMSW__
    bool MSWHandleMessage(WXLRESULT *result, WXUINT message, WXWPARAM wParam, WXLPARAM lParam) override {
        switch ( message ) {
            case WM_NCHITTEST:
                *result = HTTRANSPARENT;
                return true;
        }

        return wxPanel::MSWHandleMessage(result, message, wParam, lParam);
    }
#endif // __WXMSW__
};

class MyFrame : public wxFrame
{
public:
    MyFrame() {
        // Note: use Create() and not the base class constructor to ensure that
        // our overridden MSWHandleMessage() is called for WM_NCCALCSIZE
        // generated during the window creation.
        Create(nullptr, wxID_ANY, "Window without standard chrome");

#ifdef __WXMSW__
        const HWND hwnd = GetHandle();

        // Give the window the smallest possible frame to show the shadow.
        MARGINS margins = { 0 };
        margins.cyTopHeight = 1;

        HRESULT hr;

        hr = ::DwmExtendFrameIntoClientArea(hwnd, &margins);
        if ( FAILED(hr) )
            wxLogDebug("DwmExtendFrameIntoClientArea failed with error %08x.", hr);

        DWORD policy = DWMNCRP_ENABLED;
        hr = ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        if ( FAILED(hr) )
            wxLogDebug("DwmSetWindowAttribute(DWMWA_NCRENDERING_POLICY) failed with error %08x.", hr);
#endif // __WXMSW__

        auto p = new MyPanel(this);
        auto s = new wxBoxSizer(wxVERTICAL);

        s->AddStretchSpacer();

        auto l = new wxStaticText(p, wxID_ANY, "Hello");
        l->SetFont(l->GetFont().Bold().Scaled(2));
        s->Add(l, wxSizerFlags().Border().Center());

        auto b = new wxButton(p, wxID_CLOSE);
        b->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
        s->Add(b, wxSizerFlags().Border().Center());

        s->AddStretchSpacer();

        p->SetSizer(s);
    }

#ifdef __WXMSW__
protected:
    void HandleNCCalcSize(WPARAM wParam, LPARAM lParam) {
        const auto rect = reinterpret_cast<RECT*>(lParam);

        const auto orig = *rect;

        // We need to call the default window proc as it has some side effects,
        // e.g. without doing it tile/cascade windows doesn't work.
        MSWDefWindowProc(WM_NCCALCSIZE, wParam, lParam);

        *rect = orig;
    }

    LRESULT HandleNCHitTest(int x, int y) {
        ScreenToClient(&x, &y);

        const auto border = wxGetSystemMetrics(SM_CXFRAME, this) + wxGetSystemMetrics(SM_CXPADDEDBORDER, this);

        enum Part {
            Start,
            Middle,
            End
        };

        // Lookup table index by Part enum above.
        const int values[3][3] = {
            { HTTOPLEFT,    HTTOP,    HTTOPRIGHT    },
            { HTLEFT,       HTCLIENT, HTRIGHT       },
            { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT }
        };

        const auto getPart = [border](int value, int size) -> Part {
            return value < border ? Start
                                  : value <= size - border ? Middle
                                                           : End;
        };

        const auto size = GetClientSize();

        return values[getPart(y, size.y)][getPart(x, size.x)];
    }

    bool MSWHandleMessage(WXLRESULT *result, WXUINT message, WXWPARAM wParam, WXLPARAM lParam) override {
        switch ( message ) {
            case WM_LBUTTONDOWN:
                // Allow window dragging from any point when it's not maximized.
                if ( IsMaximized() )
                    return wxFrame::MSWHandleMessage(result, message, wParam, lParam);

                ReleaseCapture();
                SendMessage(GetHandle(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
                break;

            case WM_NCCALCSIZE:
                // Make the client area the same size as the window.
                HandleNCCalcSize(wParam, lParam);
                break;

            case WM_NCHITTEST:
                // When the window is maximized, it doesn't allow resizing.
                *result = IsMaximized()
                            ? HTCLIENT
                            : HandleNCHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                break;

            default:
                // No special processing, let the default handling take place.
                return wxFrame::MSWHandleMessage(result, message, wParam, lParam);
        }

        return true;
    }
#endif // __WXMSW__
};

class MyApp : public wxApp
{
public:
    bool OnInit() override {
        if ( !wxApp::OnInit() )
            return false;

        auto f = new MyFrame();
        f->Show();

        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
