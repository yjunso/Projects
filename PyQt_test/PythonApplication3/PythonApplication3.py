import sys
from PyQt4.QtGui import *
from PyQt4.QtCore import *
from PyQt4.QAxContainer import *


class MyWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("PyStock")
        self.setGeometry(300,300,300,300)

        self.text_status = QTextEdit(self)
        self.text_status.setGeometry(150, 20, 140, 30)
        self.text_status.setEnabled(False)

        self.text_edit = QTextEdit(self)
        self.text_edit.setGeometry(10, 60, 280, 210)
        self.text_edit.setEnabled(False)

        self.kiwoom = QAxWidget("KHOPENAPI.KHOpenAPICtrl.1")

        if self.kiwoom.dynamicCall("GetConnectState()") == 0:
            self.text_status.append("Not Connected")

        btn1 = QPushButton("Login", self)
        btn1.move(20,20)
        btn1.clicked.connect(self.btn1_clicked)



    def btn1_clicked(self):
        self.kiwoom.dynamicCall("CommConnect()")
        self.kiwoom.connect(self.kiwoom, SIGNAL("OnEventConnect(int)"), self.OnEventConnect)

        if self.kiwoom.dynamicCall("GetConnectState()") == 0:
            self.statusBar().showMessage("Not Connected")
        else:
            self.statusBar().showMessage("Connected")

    def OnEventConnect(self, ErrCode):
        if ErrCode == 0:
            self.text_edit.append("로그인 성공")
        
if __name__ == "__main__":
    app = QApplication(sys.argv)
    myWindow = MyWindow()
    myWindow.show()
    app.exec_()