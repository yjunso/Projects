﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using KiwoomCode;


namespace KOASampleCS
{
    public partial class CondListForm2 : Form
    {
        Form1 Parent;

        public CondListForm2()
        {
            InitializeComponent();
            //this.MdiParent = Parent;
        }

        public CondListForm2(Form1 _Form1)
        {
            InitializeComponent();
            Parent = _Form1;
            Parent.Logger(Log.조회, "새창띄움");
        }

        private void CondListForm2_Load(object sender, EventArgs e)
        {
            string strConList;
            int m_nRet;
            //Parent = sender.
            //Form1 CForm1 = new Form1();
            //this.MdiParent.Logger(Log.조회, "새창띄움");

            m_nRet = Parent.Form1_CondListLoad();
            //MdiParent.Logger(Log.조회, m_nRet.ToString());

            SetupDataGridView();

            if (m_nRet > 0)
            {
                strConList = Parent.Form1_CondNameLoad();

                //MdiParent.Logger(Log.조회, strConList);

                string[] spConList = strConList.Split(';');

                // ComboBox 출력
                for (int i = 0; i < spConList.Length; i++)
                {
                    if (spConList[i].Trim().Length >= 2)
                    {
                        string[] spCon = spConList[i].Split('^');
                        int nIndex = Int32.Parse(spCon[0]);
                        string strConditionName = spCon[1];
                        cbo조건식.Items.Insert(nIndex, strConditionName);
                    }
                }
            }
            // 분리된 문자 배열 저장
            /*
            if (Parent.axKHOpenAPI.GetConnectState() == 0)
            {
                Parent.Logger(Log.일반, "Open API 연결 : 미연결");
            }
            else
            {
                Parent.Logger(Log.일반, "Open API 연결 : 연결중");
            }
            */

            if (cbo조건식.Items.Count > 0)
                cbo조건식.SelectedIndex = 0;
        }

        public void SetupDataGridView()
        {
            this.Controls.Add(RealAddGridView1);
            RealAddGridView1.ColumnCount = 6;

            RealAddGridView1.Columns[0].Name = "코드";
            RealAddGridView1.Columns[1].Name = "종목명";
            RealAddGridView1.Columns[2].Name = "현재가";
            RealAddGridView1.Columns[3].Name = "전일대비 등락율";
            RealAddGridView1.Columns[4].Name = "거래량";
            RealAddGridView1.Columns[5].Name = "52주 고가";

            // Resize the height of the column headers. 
            RealAddGridView1.AutoResizeColumnHeadersHeight();

            // Resize all the row heights to fit the contents of all non-header cells.
            RealAddGridView1.AutoResizeRows(
                DataGridViewAutoSizeRowsMode.AllCellsExceptHeaders);

        }

        private void button1_Click(object sender, EventArgs e)
        {
            int lRet;
            Parent.Form1Logger("Form1Logger() : " + cbo조건식.SelectedText);
            
            if (cbo조건식.Items.Count > 0)
            {
                //cbo조건식.SelectedIndex;
                lRet = Parent.axKHOpenAPI.SendCondition(Parent.GetScrNum(),
                                                  cbo조건식.Text,
                                                  cbo조건식.SelectedIndex,
                                                  0);

                if (lRet == 1)
                {
                    Parent.Logger(Log.일반, "조건식 일반 조회 실행이 성공 되었습니다");
                }
                else
                {
                    Parent.Logger(Log.에러, "조건식 일반 조회 실행이 실패 하였습니다");
                }
            }

        }

        private void RealAddGridView1_CellContentClick(object sender, DataGridViewCellEventArgs e)
        {

        }
    }
}
