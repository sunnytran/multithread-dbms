
// please compile with lpthread

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>

using namespace std;

vector<int*> users;
int startGroup, startGroupCount, currGroupCount;
int waitGroupCount = 0, waitDbCount = 0;
pthread_mutex_t muts[2];
int dbpos[10 + 1] = {0};
pthread_cond_t pct = PTHREAD_COND_INITIALIZER, conds[10 + 1] = {PTHREAD_COND_INITIALIZER};

void crit(void (*x)(int *, int), int *user, int mutIndex) {
	pthread_mutex_lock(&muts[mutIndex]);
	x(user, mutIndex);
	pthread_mutex_unlock(&muts[mutIndex]);
}

void waitGroup(int *user, int mutIndex) {
	cout << "User " << user[0] << " is waiting due to its group" << endl;
	++waitGroupCount;
	pthread_cond_wait(&pct, &muts[mutIndex]);
}

void waitDb(int *user, int mutIndex) {
	cout << "User " << user[0] << " is waiting: position " << user[2]
		<< " of the database is being used by user " << dbpos[user[2]] << endl;
	++waitDbCount;
	pthread_cond_wait(&conds[user[2]], &muts[mutIndex]);
}

void access(int *user, int mutIndex) {
	cout << "User " << user[0] << " is accessing the position " << user[2]
	<< " of the database for " << user[4] << " second(s)" << endl;
	dbpos[user[2]] = user[0];
}

void mark(int *user, int mutIndex) {
	--currGroupCount;
	dbpos[user[2]] = 0;
	pthread_cond_signal(&conds[user[2]]);
}

void pctSignal(int *user, int mutIndex) {
	pthread_cond_signal(&pct);
}

void *x(void *args) {
	int *user = (int *)args;

	sleep(user[3]);
	cout << "User " << user[0] << " from Group " << user[1] << " arrives to DBMS" << endl;

	if (startGroupCount > 0 && user[1] != startGroup)
		crit(waitGroup, user, 0);
	if (dbpos[user[2]] != 0)
		crit(waitDb, user, 1);
	crit(access, user, 1);
	sleep(user[4]);
	cout << "User " << user[0] << " finished its execution" << endl;
	crit(mark, user, 1);

	if (currGroupCount == 0) {
		cout << endl;
		cout << "All users from Group " << startGroup << " finished their execution" << endl;
		cout << "The users from Group " << startGroup % 2 + 1 << " start their execution" << endl;
		cout << endl;

		for (int i = users.size() - startGroupCount; i >= 1; i--)
			crit(pctSignal, user, 0);
	}

	pthread_exit(NULL);
}

int main() {
	string line;
	getline(cin, line);
	startGroup = stoi(line);

	for (int i = 0; i < 2; i++)
		pthread_mutex_init(&muts[i], NULL);

	int group, pos, arrTime, reqTime;
	int count = 1;
	while (cin >> group >> pos >> arrTime >> reqTime) {
		if (group == startGroup)
			++startGroupCount;
		users.push_back(new int[5]{count++, group, pos, arrTime, reqTime});
	}
	currGroupCount = startGroupCount;

	pthread_t ths[users.size()];
	for (int i = 0; i < users.size(); i++) {
		int th = pthread_create(&ths[i], NULL, x, users.at(i));
		if (i > 0)
			users.at(i)[3] += users.at(i-1)[3];
	}
	for (int i = 0; i < users.size(); i++)
		pthread_join(ths[i], NULL);	
	cout << endl;	

	cout << "Total Requests:" << endl;
	if (startGroup == 1) {
		cout << "\t Group 1: " << startGroupCount << endl;
		cout << "\t Group 2: " << users.size() - startGroupCount << endl;
	} else {
		cout << "\t Group 1: " << users.size() - startGroupCount << endl;
		cout << "\t Group 2: " << startGroupCount << endl;
	}
	cout << endl;	

	cout << "Requests that waited:" << endl;
	cout << "\tDue to its group: " << waitGroupCount << endl;
	cout << "\tDue to a locked position: " << waitDbCount << endl;
}

