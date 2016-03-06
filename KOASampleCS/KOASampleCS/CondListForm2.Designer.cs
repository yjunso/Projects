namespace KOASampleCS
{
    partial class CondListForm2
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.cbo조건식 = new System.Windows.Forms.ComboBox();
            this.button1 = new System.Windows.Forms.Button();
            this.RealAddGridView1 = new System.Windows.Forms.DataGridView();
            ((System.ComponentModel.ISupportInitialize)(this.RealAddGridView1)).BeginInit();
            this.SuspendLayout();
            // 
            // cbo조건식
            // 
            this.cbo조건식.FormattingEnabled = true;
            this.cbo조건식.Location = new System.Drawing.Point(3, 3);
            this.cbo조건식.Name = "cbo조건식";
            this.cbo조건식.Size = new System.Drawing.Size(121, 21);
            this.cbo조건식.TabIndex = 0;
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(130, 3);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 1;
            this.button1.Text = "조건명 호출";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // RealAddGridView1
            // 
            this.RealAddGridView1.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.RealAddGridView1.Location = new System.Drawing.Point(3, 32);
            this.RealAddGridView1.Name = "RealAddGridView1";
            this.RealAddGridView1.Size = new System.Drawing.Size(533, 225);
            this.RealAddGridView1.TabIndex = 2;
            this.RealAddGridView1.CellContentClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.RealAddGridView1_CellContentClick);
            // 
            // CondListForm2
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(540, 261);
            this.Controls.Add(this.RealAddGridView1);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.cbo조건식);
            this.Font = new System.Drawing.Font("맑은 고딕", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "CondListForm2";
            this.Text = "조건검색";
            this.Load += new System.EventHandler(this.CondListForm2_Load);
            ((System.ComponentModel.ISupportInitialize)(this.RealAddGridView1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ComboBox cbo조건식;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.DataGridView RealAddGridView1;
    }
}