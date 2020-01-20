#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <Eigen/Dense>

// This is a SVM
// Basic on Li Hang ��ͳ��ѧϰ������

typedef Eigen::VectorXf EigenVectorFloat;
typedef Eigen::MatrixXf EigenMatrixFloat;
typedef Eigen::VectorXi EigenVectorInt;

// ����������
typedef std::vector<float> VectorFloat;
// ��������
typedef std::vector<int> VectorInt;
// �����;���
typedef std::vector<VectorFloat> FloatMat;
// ���ξ���
typedef std::vector<VectorInt> IntMat;

// ���ļ��ж�ȡ���ݼ������ǩ
// ����:�ļ�·��, ��ȡ����
void LoadDate(const char *filename, int line, int col,
             EigenMatrixFloat &datamat, EigenVectorInt &labellist) {

  labellist = EigenVectorInt(line);
  datamat = EigenMatrixFloat(line, col);
  // ���ļ�
  std::ifstream inFile(filename, std::ios::in);
  if (inFile.fail()) {
    puts("��ȡ�ļ�ʧ�ܣ�");
  }
  // CSV�ļ���һ�����ݵ�����
  std::string lineStr;
  // ���ļ��ж�ȡһ�����ݵ�lineStr��
  int i = 0;
  int lines = -1;
  while (getline(inFile, lineStr) && line--) {
    // str ����װ���Ÿ���������
    std::string str;
    // ��һ���ο����ϵ�
    std::stringstream ss(lineStr);
    // ����һ�����ݶ�ȡ�ı�־λ
    int start = 0;
    int cols = 0;
    // �ָ�lineStr�������ݶ��Ÿ�����ÿһ���ֱ�洢
    while (getline(ss, str, ',')) {
      // ��������еĵ�һ�����֣���ǩ��
      if (!start) {
        // atoi <stdlib.h> ���ַ����е�����ת��Ϊint��
        int label = atoi(str.c_str());
        if (label == 0) {
          labellist(i) = 1;
          printf("��%d��ǩ��1\n", ++i);
        }
        else {
          labellist(i) = -1;
          printf("��%d��ǩ��-1\n", ++i);
        }
        // ��������
        ++lines;
        start = 1;
        continue;
      }
      float data = atof(str.c_str());
      datamat(lines, cols) = data;
      ++cols;
    }
  }
  printf("��������������%d\n", i);
}

// SVM��
class SVM {

public:
  // SVM��ز�����ʼ��
  // traindatalist:ѵ�������ݼ�
  // trainlabellist:ѵ���ñ�ǩ��
  // sigma:��˹���еķ�ĸsigma
  // C:�ͷ�ϵ��
  // toler:�ɳڱ���
  SVM(EigenMatrixFloat &datalist, EigenVectorInt &labellist,
    float sigma, float C, float toler) {
    traindatalist = datalist;
    trainlabellist = labellist;
    if (!datalist.size()) {
      puts("data is NULL!!");
      return;
    }
    // ��һ��һ��Ҫ�п�
    m = datalist.rows();
    n = datalist.cols();
    sigmas = sigma;
    Cs = C;
    tolers = toler;
    b = 0;

    CalcKernal();
    // �����е�alpha����ֵΪ0������Ϊѵ������Ŀ
    alpha = VectorFloat(m, 0);
    // �����е�EiҲ����ֵΪ0������Ϊѵ������Ŀ
    E = VectorFloat(m, 0);
    // ֧����������
    //supportvecindex = FLOATVECTOR(m, 0);
  }

  // �˺�������
  // ʹ�õ��Ǹ�˹�˺����������7.3.3 ���ú˺����� ʽ7.90
  // ����������kernelanswer������
  void CalcKernal() {
    // ��ʼ����˹�˽�������С = ѵ�����ĳ��ȵ�ƽ��
    // ����m��������ÿ����������m��Ԫ��
    // kernelanswer[i][j] = xi * xj;
    kernelanswer = FloatMat(m, VectorFloat(m, 0));
    float result;

    // ��ѭ������xi
    for (int i = 0; i < m; ++i) {
      // ����xi
      EigenVectorFloat X = traindatalist.row(i);
      // Сѭ��������ֻ�ü���һ�뼴��
      for (int j = i; j < m; ++j) {
        EigenVectorFloat Z = traindatalist.row(j);
        // ||X-Z||^2
        result = (X - Z).squaredNorm();
        result = exp(-1 * result / (2 * sigmas * sigmas));
        kernelanswer[i][j] = result;
        kernelanswer[j][i] = result;
      }
    }
  }

  // kkt���������ж�
  // �鿴��i��alpha�Ƿ�����kkt����
  // return true,���㣬false��������
  bool IsSatisfyKKT(int i) {
    // ���g(x)
    float gxi = CalcGxi(i);
    int yi = trainlabellist(i);
    //printf("i:%d gxi:%f yi:%d\n",i,gxi,yi);

    // ���ݡ�7.4.2 ������ѡ�񷽷����С���һ��������ѡ��
    // 7.111 �� 7.113
    if ((fabs(alpha[i]) < tolers) && (yi * gxi >= 1)) {
      return true;
    }
    else if ((fabs(alpha[i] - Cs) < tolers) && (yi * gxi <= 1)) {
      return true;
    }
    else if ((alpha[i] > -tolers) && (alpha[i] < (Cs + tolers)) && \
      (fabs(yi * gxi - 1) < tolers)) {
      return true;
    }
    return false;
  }

  // ����i�㵽��ƽ�����
  // ����7.104ʽ��
  // return ����
  float CalcGxi(int i) {
    /*
    ��Ϊg(xi)��һ�����ʽ + b����ʽ����ͨ����Ӧ����ֱ��������ʽ�е�ÿһ������Ӽ���
    ���Ƿ��֣��ڡ�7.2.3 ֧����������ͷ��һ�仰��˵������Ӧ�ڦ� > 0��������
    (xi, yi)��ʵ��xi��Ϊ֧����������Ҳ����˵ֻ��֧�������Ħ��Ǵ���0�ģ������ʽ�ڵ�
    ��Ӧ�Ħ�i*yi*K(xi, xj)��Ϊ0����֧�������Ħ�i*yi*K(xi, xj)��Ϊ0��Ҳ�Ͳ���Ҫ����
    �������С�Ҳ����˵����g(xi)�ڲ����ʽ�������У�ֻ��Ҫ����� > 0�Ĳ��֣����ಿ�ֿ�
    ���ԡ���Ϊ֧�������������ǱȽ��ٵģ����������ٺܴ�̶��Ͻ�Լʱ��
    */
    // ��ʼ��gxi
    float gxi = 0;
    // �����Ϊ��ͼ���
    for (int j = 0; j < m; ++j) {
      if (alpha[j] != 0) {
        //printf("gxi: %d\n",j);
        //printf("alpha:%f\n", alpha[j]);
        //gxi = gxi + alpha[j] * trainlabellist(j) * kernelanswer[j][i];
        int yj = trainlabellist(j);
        gxi = gxi + (alpha[j] * yj * kernelanswer[j][i]);
      }
    }
    gxi = gxi + b;
    return gxi;
  }

  // �������
  // ����ʽ�� 7.105
  float CalcEi(int i) {
    float gxi = CalcGxi(i);
    return (gxi - trainlabellist(i));
  }

  // ��õڶ���alpha
  // ��һ��alphaͨ��������KKT����ѡȡ
  // ���룺E1����һ����������i,��һ��alpha
  // ���صڶ���alpha�����
  int GetAlphaJ(float E1, int i) {
    int alpha2_index = -1;
    // �ڶ�������ѡȡ�ı�׼��E1-E2
    float maxE1_E2 = -1;
    // E�����е�Ei�ʼ����0����ֻҪ������Ϊ0��Ei����
    for (int j = 0; j < m; ++j) {
      if (E[j] != 0) {
        E[j] = CalcEi(j);
        if (maxE1_E2 < fabs(E1 - E[j])) {
          maxE1_E2 = fabs(E1 - E[j]);
          alpha2_index = j;
        }
      }
    }
    // ��E��������û�з�0�ģ����������������ѡһ��
    if (alpha2_index == -1) {
      //srand(int(time(0)));
      //int r = rand() % m;
      //while (r > m) {
      //  r = r % m;
      //}
      alpha2_index = i + 2;
    }
    //printf("the maxindex is: %d\n", alpha2_index);
    return alpha2_index;
  }

  // SVMѵ����SMO�㷨 
  // �����������
  void Train(int count) {
    // ���ݸı��־λ
    int paramerter_changed = 1;

    // ��������
    int iter = 0;
    // ����Ѿ��ﵽ���������������Ѿ�û�в����ı�-->��Ϊ�Ѿ��ﵽ����״̬
    while (iter < count && paramerter_changed > 0) {
      // ��ӡ��ǰ��������
      printf("iter: %d \n", iter);
      iter += 1;
      paramerter_changed = 0;

      // һ�ε���Ҫ�������е�alpha
      // ��ѭ���ҳ���һ������
      for (int k = 0; k < m; ++k) {
        // ���������KKT��������ѡ������alpha
        if (!IsSatisfyKKT(k)) {
          //printf("K:%d", k);
          // �ȼ���E1,alpha2newnew = alpha2old + yi(E1 - E2)/(K11 + K22 - 2K12��
          float E1 = CalcEi(k);
          //printf("i = %d, E1 = %\n", k, E1);
          // ѡȡ�ڶ���alpha
          int j = GetAlphaJ(E1, k);
          float E2 = CalcEi(j);
          // ���Ե����� k �ǵ�һ��alpha���±�
          // j �ǵڶ���alpha���±�

          // ����alpha2new��L��H
          // ��y1 != y2
          float L = 0, H = 0;
          if (trainlabellist(k) != trainlabellist(j)) {
            L = fmax(0, alpha[j] - alpha[k]);
            H = fmin(Cs, Cs + alpha[j] - alpha[k]);
          }
          else {
            L = fmax(0, alpha[k] + alpha[j] - Cs);
            H = fmin(Cs, alpha[k] + alpha[j]);
          }
          //���L��H��ȣ�˵���������Ż���������ǰѭ����������һ������
          if (L == H) {
            continue;
          }
          // ����alpha2new_new
          float K11 = kernelanswer[k][k];
          float K12 = kernelanswer[k][j];
          float K21 = kernelanswer[j][k];
          float K22 = kernelanswer[j][j];

          float alpha2new_new = alpha[j] + trainlabellist(j)  * (E1 - E2) \
            / (K11 + K22 - 2 * K12);
          // ����alpha2new_new��ֵ��alpha2
          if (alpha2new_new < L) {
            alpha2new_new = L;
          }
          else if (alpha2new_new > H) {
            alpha2new_new = H;
          }
          // ����alpha1new
          // ����ʽ�� 7.109
          float alpha1new = alpha[k] + trainlabellist(k) * trainlabellist(j) \
            * (alpha[j] - alpha2new_new);

          // ����b
          // ���ݡ�7.4.2 ������ѡ�񷽷���������ʽ7.115��7.116����b1��b2
          float b1 = -1 * E1 - trainlabellist(k) * K11 * (alpha1new - alpha[k])\
            - trainlabellist(j) * K21 * (alpha2new_new - alpha[j]) + b;
          //printf("b1: %f\n", b1);
          float b2 = -1 * E2 - trainlabellist(k) * K12 * (alpha1new - alpha[k])\
            - trainlabellist(j) * K22 * (alpha2new_new - alpha[j]) + b;
          //printf("b2: %f\n", b2);

          // ����alpha1��alpha2��ֵ��Χȷ���µ�b
          if (alpha1new > 0 && alpha1new < Cs) {
            b = b1;
          }
          else if (alpha2new_new > 0 && alpha2new_new < Cs) {
            b = b2;
          }
          else {
            b = (b1 + b2) / 2;
          }

          // ����Ķ���С�Ͳ�����parameter_changed
          if (fabs(alpha2new_new - alpha[j]) >= 0.00001) {
            paramerter_changed += 1;
          }

          // ����alpha��E
          alpha[k] = alpha1new;
          alpha[j] = alpha2new_new;
          E[k] = CalcEi(k);
          E[j] = CalcEi(j);
        }
      }
      printf("��������:%d���ı�alpha����:%d\n", iter, paramerter_changed);
    }
    printf(" b = %f\n", b);
    // �������е�֧������������
    for (int s = 0; s < m; ++s) {
      if (alpha[s] > 0) {
        supportvecindex.push_back(s);
      }
    }
  }

  // ��Ԥ���ʱ����Ҫ�����ļ���˺���
  float PredictCalcSingleKernel(int i, int j) {
    // ����xi��ѵ�����е�֧������,����֧����������������
    EigenVectorFloat X = traindatalist.row(i);
    // ��������
    EigenVectorFloat Z = testdatalist.row(j);
    // ||X-Z||^2
    float a = 0;
    /*for (int s = 0; s < n; ++s) {
       a += (X[s] - Z[s]) * (X[s] - Z[s]);
    }*/
    a = (X - Z).squaredNorm();
    //�������˹�˽��
    a = exp(-a / (2 * sigmas * sigmas));
    return a;
  }

  // Ԥ��
  int predict(int i) {
    float K_p = 0, result = 0;
    // �������е�֧��������������͹�ʽ
    for (int k = 0; k < supportvecindex.size(); ++k) {
      // �ȵ����Ľ�Ԥ��ĺ˺��������
      // ����֧�������������Ͳ�������������
      // alpha[k:n] * y[k:n] * K[k:n,i]
      K_p = PredictCalcSingleKernel(supportvecindex[k], i);
      // �����
      result = result + K_p * trainlabellist(supportvecindex[k]) * alpha[supportvecindex[k]];
    }
    result += b;

    // sign()
    if (result < 0) {
      return (-1);
    }
    else {
      return (1);
    }
  }

  // ����ѵ���õ�ģ��
  void Test() {
    int error_count = 0;
    // �������Լ���������
    for (int i = 0; i < testdatalist.rows(); ++i) {
      printf("Ԥ����Ϊ�� %d\n", predict(i));
      int r = predict(i);
      //printf("Ԥ����Ϊ��%d\n", r);
      if (r != testlabellist(i)) {
        printf("����������%d ��Ԥ���ǩΪ��%d ,ԭ����ǩΪ��%d\n", i, r, testlabellist(i));
        error_count += 1;
      }
    }
    printf("����ĸ�����%d\n", error_count);
    printf("��ȷ��: %f\n", 1 - (float)error_count / (float)testdatalist.rows());
  }

  // ����ģ��
  bool SaveModel() {
    std::ofstream outFile;
    outFile.open("\model.csv", std::ios::out | std::ios::trunc);
    if (outFile.is_open()) {
      for (int i = 0; i < supportvecindex.size(); ++i) {
        outFile
          << trainlabellist(supportvecindex[i]) << ",";
        for (int j = 0; j < n; ++j) {
          outFile
            << traindatalist(supportvecindex[i], j) << ",";
        }
        outFile
          << alpha[supportvecindex[i]] << std::endl;
      }
      outFile << b << std::endl;
      outFile.close();
      return true;
    }
    return false;
  }

  // ��ȡģ��
  // ���룺ģ��·����֧�������ĸ�������������ֵ����
  void GetModel(const char *filename, int line, int col,
    EigenMatrixFloat &datamat, EigenVectorInt &labellist) {
    labellist = EigenVectorInt(line);
    datamat = EigenMatrixFloat(line, col);
    // ���ļ�
    std::ifstream inFile(filename, std::ios::in);
    if (inFile.fail()) {
      puts("��ȡ�ļ�ʧ�ܣ�");
    }
    // CSV�ļ���һ�����ݵ�����
    std::string lineStr;
    // ���ļ��ж�ȡһ�����ݵ�lineStr��
    int i = 0;
    int lines = -1;
    while (getline(inFile, lineStr) && line--) {
      // str ����װ���Ÿ���������
      std::string str;
      // ��һ���ο����ϵ�
      std::stringstream ss(lineStr);
      // ����һ�����ݶ�ȡ�ı�־λ
      int start = 0;
      int cols = 0;
      // ����supportvector����
      supportvecindex.push_back(i);
      // �ָ�lineStr�������ݶ��Ÿ�����ÿһ���ֱ�洢
      while (getline(ss, str, ',')) {
        // ��������еĵ�һ�����֣���ǩ��
        if (!start) {
          // atoi <stdlib.h> ���ַ����е�����ת��Ϊint��
          int label = atoi(str.c_str());
          if (label == 0) {
            labellist(i) = 1;
            printf("��%d��ǩ��1\n", ++i);
          }
          else {
            labellist(i) = -1;
            printf("��%d��ǩ��-1\n", ++i);
          }
          // ��������
          ++lines;
          start = 1;
          continue;
        }
        if (cols < col) {
          // ת����0~1֮�����
          float data = atof(str.c_str());
          datamat(lines, cols) = data;
          ++cols;
          continue;
        }
        alpha[lines] = atof(str.c_str());
      }
    }
    // ����B
    b = 0;
    printf("��������������%d\n", i);
  }

  VectorInt supportvecindex;    // ֧����������
  VectorFloat alpha;            // �������ճ�������
  VectorFloat E;                // SMO�㷨��������е�E������
  EigenMatrixFloat traindatalist;       // ѵ�����ݼ�
  EigenVectorInt trainlabellist;     // ��ǩ��
  EigenMatrixFloat testdatalist;        // �������ݼ�
  EigenVectorInt testlabellist;      // ���Ա�ǩ��
  FloatMat kernelanswer;        // �˺����������
  int m;                        // ���ݼ�����  
  int n;                        // ������������
  float b;                      // SVM�е�ƫ��
private:
  float sigmas;                 // ��˹���еķ�ĸ
  float Cs;                     // �ͷ�ϵ��
  float tolers;                 // �ɳڱ���
};


void main() {
  // ���ݼ��ͱ�ǩ������
  EigenMatrixFloat datalist, testlist;
  EigenVectorInt labellist, testlabellist;

  // �����ݼ��е������ݺͱ�ǩ
  LoadDate("C:\\Users\\shuangjiang ou\\Desktop\\���ݼ�\\newtrain_data.csv", 311, 100, datalist,labellist);
  //LoadDate("C:\\Users\\shuangjiang ou\\Desktop\\data_test.csv", 1800, 3, datalist,labellist);
  // �����ݼ��е���������ݺͱ�ǩ
  LoadDate("C:\\Users\\shuangjiang ou\\Desktop\\���ݼ�\\newtrain_data.csv", 311, 100, testlist, testlabellist);
  //LoadDate("C:\\Users\\shuangjiang ou\\Desktop\\data_test.csv", 1800, 3, testlist, testlabellist);
  // LoadDate("C:\\Users\\shuangjiang ou\\Desktop\\Mnist\\mnist_train.csv", 200, testlist, testlabellist);
  SVM svm(datalist, labellist, 10, 100, 0.04);
  svm.testdatalist = testlist;
  svm.testlabellist = testlabellist;
  
  svm.Train(1000);
  svm.Test();
  svm.SaveModel();
  int i;
  scanf("%d", &i);

  //datalist = FLOATMAT(4, FLOATVECTOR(2));
  //labellist = INTVECTOR(4);
  //for (int i = 0; i < 4; ++i) {
  //  printf("�����%d��������\n", i);
  //  float j = 0;
  //  scanf("%f", &j);
  //  datalist[i][0] = j;
  //  scanf("%f", &j);
  //  datalist[i][1] = j;
  //  puts("���������������ֵ");
  //  scanf("%f", &j);
  //  labellist[i] = j;
  //}
  //// ��ʼ��һ��SVM
  //SVM svm(datalist, labellist, 10, 200, 0.001);
  //svm.Train(1000);
  //for (int i = 0; i < svm.m; ++i) {
  //  for (int j = 0; j < svm.m; ++j) {
  //    printf("%f\t", svm.kernelanswer[i][j]);
  //  }
  //  printf("\n");
  //}
  //testlist = FLOATMAT(4, FLOATVECTOR(2));
  //for (int i = 0; i < 4; ++i) {
  //  printf("�����%d��������\n", i);
  //  float j = 0;
  //  scanf("%f", &j);
  //  testlist[i][0] = j;
  //  scanf("%f", &j);
  //  testlist[i][1] = j;
  //}
  //svm.testdatalist = testlist;
  //svm.Test();
  //int j;
  //scanf("%d", &j);
}
