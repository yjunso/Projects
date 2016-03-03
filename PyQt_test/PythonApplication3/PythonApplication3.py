import sys
from PyQt4.QtGui import *
from PyQt4.QtCore import *
from PyQt4.QAxContainer  import *


class MyWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("PyStock")
        self.setGeometry(300,300,300,400)

        btn1 = QPushButton("Click me", self)
        btn1.move(20,20)
        btn1.clicked.connect(self.btn1_clicked)

        btn2 = QPushButton("Click me too", self)
        btn2.move(40,40)
        btn2.clicked.connect(self.btn2_clicked)

    def btn1_clicked(self):
        QMessageBox.about(self, "message", "clicked")
    
    def btn2_clicked(self):
        QMessageBox.about(self, "message", "clicked")
        
if __name__ == "__main__":
    app = QApplication(sys.argv)
    myWindow = MyWindow()
    myWindow.show()
    app.exec_()

app = QApplication(sys.argv)
print(sys.argv)
label = QLabel("Hello PyQt")
label.show()
app.exec_()