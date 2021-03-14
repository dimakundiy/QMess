import QtQuick 2.6
import QtQuick.Controls 2.0

Item {
    id: unicodeIcon

    property alias unicodeText: unicodeTxt.text
    property alias fontSize: unicodeTxt.font.pointSize

    Text {
        id: unicodeTxt
        //    text: "\uf023"
        font.pointSize: 14
        font.family: "fontawesome"
        color: mainAppColor
        anchors.centerIn: parent
    }
}
/*##^##
Designer {
    D{i:0;autoSize:true;formeditorColor:"#4c4e50";formeditorZoom:0.9;height:480;width:640}
}
##^##*/
