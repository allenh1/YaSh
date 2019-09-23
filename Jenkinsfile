    node {
        def app

        stage('Clone') {
            checkout scm
            sh 'git clean -xdf'
        }

        stage('Test Compile') {
                sh 'cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build'
        }

        stage('Static Analyzer') {
            echo 'Analyzing with PVS Studio...'
            sh 'rm -rf build && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo && pvs-studio-analyzer trace -- make -j65'
            sh '~/run-pvs.bash build'
            sh 'python3 ~/pvs-log-to-md.py build/pvs.tasks > index.html'
            catchError(buildResult: 'SUCCESS', stageResult: 'FAILURE') {
                sh '~/check-pvs-build.bash build/pvs.tasks'
            }
            publishHTML([allowMissing: false, alwaysLinkToLastBuild: false, keepAll: false, reportDir: '', reportFiles: 'index.html', reportName: 'PVS-Studio Report', reportTitles: ''])
        }
}
