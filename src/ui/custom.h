#include <wx/wx.h>

class TxtBox : public wxTextCtrl {
public:
    TxtBox(wxWindow *parent, wxWindowID id, const wxString &value = wxEmptyString,
           const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize,
           long style = 0, const wxValidator &validator = wxDefaultValidator,
           const wxString &name = wxTextCtrlNameStr)
        : wxTextCtrl(parent, id, value, pos, size, style | wxBORDER_NONE, validator, name)
    {
    }
};