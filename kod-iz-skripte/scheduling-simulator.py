# Simple task scheduling simulator for RMPA, EDF(+RMPA), LLF(+RMPA)

# How to use it?
# - Define scheduler by setting 'scheduler' variable
# - Define number of processors in PROC_NUM
# - Enter task system with T[] and C[]

import time, sys

RMPA = 1
EDF = 2
LLF = 3
scheduler = RMPA # choose scheduler here

t = 0
SIM_TIME = 300
PROC_NUM = 3

# tasks statically defined here with T (period) and C (computation time)
# periods must be different and sorted in increasing order
T = [5, 10, 15, 20, 25, 30]
C = [2, 6, 7, 11, 12, 15]
TASK_NUM = len(T)

# initial attributes derived from C and T
R = C[:] 		# remaining computation time
A = [0] * TASK_NUM	# when next period starts
D = T[:]		# deadline
L = [D[i] - t - R[i] for i in range(TASK_NUM)]	# laxity

# processors initially unused
active_task = [-1] * PROC_NUM

def RMPA_get_to_run():
	# ready tasks => those with A <=t and R > 0
	ready = [task for task in range(TASK_NUM) if A[task] <= t and R[task] > 0]

	# get just first PROC_NUM tasks
	return ready[:PROC_NUM]

def EDF_get_to_run():
	# ready tasks => those with A <=t and R > 0
	# get list with [task_id, deadline, arrive_time] elements
	ready = [[task, D[task], A[task]] for task in range(TASK_NUM) if A[task] <= t and R[task] > 0]

	# sort by deadline, and RMPA (id) as second criteria when deadline is the same
	ready = sorted(ready, key=lambda task: (task[1], task[0]))
	#for arrival_time as second criteria use task[2] in place of task[0] in previous line

	# get just first PROC_NUM tasks
	return [task[0] for task in ready[:PROC_NUM]]

def LLF_get_to_run():
	# ready tasks => those with A <=t and R > 0
	# get list with [task_id, laxity, arrive_time] elements
	ready = [[task, L[task], A[task]] for task in range(TASK_NUM) if A[task] <= t and R[task] > 0]

	# sort by laxity, and index as second criteria when deadline is the same
	ready = sorted(ready, key=lambda task: (task[1], task[0]))
	#for arrival_time as second criteria use task[2] in place of task[0] in previous line

	# get just first PROC_NUM tasks
	return [task[0] for task in ready[:PROC_NUM]]

#choose 'get_to_run' method based on chosen 'scheduler'
get_to_run = [RMPA_get_to_run, EDF_get_to_run, LLF_get_to_run][scheduler-1]

def schedule():
	to_run = get_to_run()

	# keep tasks that are running and in 'to_run' on same processors
	for task in range(PROC_NUM):
		if active_task[task] in to_run:
			to_run.remove(active_task[task]) # already assigned
		else:
			active_task[task] = -1

	# assign remaining tasks to free processors
	for task in to_run:
		k = active_task.index(-1)
		active_task[k] = task

# simulate execution for one time unit - update tasks that were running (and laxity for all)
def simulate_step():
	global t, L
	t += 1

	# update R for active tasks
	for proc in range(PROC_NUM):
		task = active_task[proc]
		if task == -1:
			continue

		R[task] -= 1
		if R[task] == 0:
			active_task[proc] = -1
			A[task] += T[task] # next activation
			D[task] += T[task] # next deadline
			R[task] = C[task]

	# check for deadline overruns
	for task in range(TASK_NUM):
		if A[task] <= t and R[task] > 0 and D[task] <= t:
			print("t = " + str(t) + ": Task " + str(task+1) + " missed deadline")
			sys.exit(1)

	L = [D[i] - t - R[i] for i in range(TASK_NUM)] # recalculate laxity

def print_tasks():
	print("t = " + str(t) + "\t", end="")
	for task in active_task:
		if task != -1:
			print(str(task+1), end=" ")
		else:
			print("-", end=" ")

	print("\t", end="")
	for i in range(TASK_NUM):
		if A[i] <= t and R[i] > 0:
			print("{" + str(i+1) + ": A=" + str(A[i]) + " D=" + str(D[i]) + " L=" + str(L[i]) + " R=" + str(R[i]) + "}", end="")

	print()

def main():
	method = ["RMPA", "EDF", "LLF"]
	print("Starting scheduling simulation with method: " + method[scheduler-1])
	print("time\tactive tasks\tready tasks data")
	print_tasks()
	schedule()
	#time.sleep(1)

	while t < SIM_TIME:
		print_tasks()
		#time.sleep(1)
		simulate_step()
		schedule()
	print_tasks()

if __name__ == "__main__":
	main()
