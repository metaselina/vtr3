import csv
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.dates import date2num
from matplotlib.collections import LineCollection
import datetime
import argparse

def load_data(data_dir, num_repeats, ignore_runs, path_indicesces):

    info = []
    path_segments = {'multis':[], 'new1':[], 'dark':[], 'new2':[]}

    for i in range(0, num_repeats):

        if (i+1) in ignore_runs:
            continue

        # if i in ignore_runs:
        #     continue

        path_segments['multis'] += [{"timestamp":[], "inliers_rgb":[]}]
        path_segments['new1'] += [{"timestamp":[], "inliers_rgb":[]}]
        path_segments['dark'] += [{"timestamp":[], "inliers_rgb":[]}]
        path_segments['new2'] += [{"timestamp":[], "inliers_rgb":[]}]

        info.append({"timestamp":[],
                     "live_id":[],
                     "priv_id":[],
                     "success":[],
                     "inliers_rgb":[],
                     "inliers_gray":[],
                     "inliers_cc":[],
                     "window_temporal_depth":[],
                     "window_num_vertices":[],
                     "comp_time":[]})

        results_dir = "{}/graph.index/repeats/{}/results".format(data_dir, i+1)
        info_file_path = "{}/info.csv".format(results_dir) 

        with open(info_file_path) as csv_file:

            csv_reader = csv.reader(csv_file, delimiter=',')
            first = True

            for row in csv_reader:

                if not first:
                    info[-1]["timestamp"] += [int(row[0])]
                    info[-1]["live_id"] += [row[1]]
                    info[-1]["priv_id"] += [row[2]]
                    info[-1]["success"] += [row[3]]
                    info[-1]["inliers_rgb"] += [float(row[4])]
                    info[-1]["inliers_gray"] += [float(row[5])]
                    info[-1]["inliers_cc"] += [float(row[6])]
                    info[-1]["window_temporal_depth"] += [row[7]]
                    info[-1]["window_num_vertices"] += [row[8]]
                    info[-1]["comp_time"] += [float(row[9])]

                    priv_id = int(row[2])
                    if (priv_id < path_indices[0]) or \
                       ((priv_id >= path_indices[1]) and (priv_id < path_indices[2])):
                        path_segments['multis'][-1]['timestamp'] += [int(row[0])]
                        path_segments['multis'][-1]['inliers_rgb'] += [float(row[4])]
                    elif (priv_id >= path_indices[0]) and (priv_id < path_indices[1]):
                        path_segments['new1'][-1]['timestamp'] += [int(row[0])]
                        path_segments['new1'][-1]['inliers_rgb'] += [float(row[4])]
                    elif ((priv_id >= path_indices[2]) and(priv_id < path_indices[3])) or \
                         (priv_id >= path_indices[4]):
                        path_segments['dark'][-1]['timestamp'] += [int(row[0])]
                        path_segments['dark'][-1]['inliers_rgb'] += [float(row[4])]
                    elif (priv_id >= path_indices[3]) and (priv_id < path_indices[4]):
                        path_segments['new2'][-1]['timestamp'] += [int(row[0])]
                        path_segments['new2'][-1]['inliers_rgb'] += [float(row[4])]

                first = False

        dt = datetime.datetime.fromtimestamp(info[-1]["timestamp"][0] / 1e9) 
        print("{}-{}".format(i+1, dt.strftime('%H:%M')))

    return info, path_segments

def plot_comp_time(avg_comp_time, times, colours, results_dir):
    
    f = plt.figure(figsize=(30, 12))
    f.tight_layout(rect=[0, 0.03, 1, 0.95])
    p = plt.bar(times, avg_comp_time, width=0.001)
    for i in range(len(p)):
        p[i].set_color(colours['day'][i])
    myFmt = matplotlib.dates.DateFormatter('%H:%M')
    ax = plt.axes()
    ax.xaxis.set_major_formatter(myFmt)

    plt.xlim([min(times) - datetime.timedelta(minutes=10), max(times) + datetime.timedelta(minutes=10)])
    plt.xlabel('Repeat time (hh:mm)', fontsize=22, weight='bold') 
    plt.ylabel('Mean computation time (ms)', fontsize=22, weight='bold')
    plt.xticks(fontsize=20) 
    plt.yticks(fontsize=20) 
    # plt.ylim([80, 190])
    plt.ylim([min(avg_comp_time) - 10, max(avg_comp_time) + 10])
    plt.title('Mean localization computation time for each repeat', fontsize=22, weight='bold')

    # legend_elements = [matplotlib.lines.Line2D([0], [0], color='C0', lw=4, label='Day1: 03.08'),
    #                    matplotlib.lines.Line2D([0], [0], color='C1', lw=4, label='Day2: 09.08')]
    legend_elements = [matplotlib.lines.Line2D([0], [0], color='C0', lw=4, label='Day1: 15.08'),
                       matplotlib.lines.Line2D([0], [0], color='C1', lw=4, label='Day2: 16.08'),
                       matplotlib.lines.Line2D([0], [0], color='C2', lw=4, label='Day3: 20.08')]  
    plt.legend(handles=legend_elements, fontsize=20);

    plt.savefig('{}/avg_comp_time.png'.format(results_dir), bbox_inches='tight', format='png')
    plt.close()

    # f = plt.figure(figsize=(30, 12))
    # f.tight_layout(rect=[0, 0.03, 1, 0.95])
    # p = plt.bar(times, avg_comp_time, width=0.001)
    # for i in range(len(p)):
    #     p[i].set_color(colours['gps'][i])
    # myFmt = matplotlib.dates.DateFormatter('%H:%M')
    # ax = plt.axes()
    # ax.xaxis.set_major_formatter(myFmt)

    # plt.xlim([min(times) - datetime.timedelta(minutes=10), max(times) + datetime.timedelta(minutes=10)])
    # plt.xlabel('Repeat time (hh:mm)', fontsize=22, weight='bold') 
    # plt.ylabel('Mean computation time (ms)', fontsize=22, weight='bold')
    # plt.xticks(fontsize=20) 
    # plt.yticks(fontsize=20) 
    # # plt.ylim([80, 190])
    # plt.ylim([min(avg_comp_time) - 10, max(avg_comp_time) + 10])
    # plt.title('Mean localization computation time for each repeat', fontsize=22, weight='bold')

    # legend_elements = [matplotlib.lines.Line2D([0], [0], color='C3', lw=4, label='GPS'),
    #                    matplotlib.lines.Line2D([0], [0], color='C2', lw=4, label='No GPS')]
    # plt.legend(handles=legend_elements, fontsize=20);


    # plt.savefig('{}/avg_comp_time_gps.png'.format(results_dir), bbox_inches='tight', format='png')
    # plt.close()

def plot_inliers_segments(avg_inliers_segments, times, colours, results_dir):

    x = date2num(times)

    # Plot average number of inliers for each run
    f = plt.figure(figsize=(30, 12))
    f.tight_layout(rect=[0, 0.03, 1, 0.95])
    p1 = plt.bar(x - 0.004, avg_inliers_segments['multis'], width=0.008) # 0.015 #0.001
    p2 = plt.bar(x + 0.004, avg_inliers_segments['new1'], width=0.008) # 0.015 #0.001
    for i in range(len(p1)):
        p1[i].set_color('C0')
        p2[i].set_color('C1')
    myFmt = matplotlib.dates.DateFormatter('%H:%M')
    ax = plt.axes()
    ax.xaxis.set_major_formatter(myFmt)

    plt.xlim([min(times) - datetime.timedelta(minutes=10), max(times) + datetime.timedelta(minutes=10)])
    plt.xlabel('Repeat time (hh:mm)', fontsize=22, weight='bold') 
    plt.ylabel('Mean number of inliers', fontsize=22, weight='bold')
    plt.xticks(fontsize=20) 
    plt.yticks(fontsize=20) 
    # plt.ylim([100, 450])
    # plt.ylim([40, 350])
    plt.ylim([min(avg_inliers_segments['new1']) - 10, max(avg_inliers_segments['multis']) + 10])
    plt.title('Mean number of inliers for each repeat - off road', fontsize=22, weight='bold')

    legend_elements = [matplotlib.lines.Line2D([0], [0], color='C0', lw=4, label='Area in training data'),
                       matplotlib.lines.Line2D([0], [0], color='C1', lw=4, label='Area outside training data')]                
    plt.legend(handles=legend_elements, fontsize=20);

    plt.savefig('{}/avg_inliers_outsidetraining_offroad.png'.format(results_dir), bbox_inches='tight', format='png')
    plt.close()

    # Plot average number of inliers for each run
    f = plt.figure(figsize=(30, 12))
    f.tight_layout(rect=[0, 0.03, 1, 0.95])
    p1 = plt.bar(x-0.004, avg_inliers_segments['dark'], width=0.008) # 0.015 #0.001
    p2 = plt.bar(x+0.004, avg_inliers_segments['new2'], width=0.008) # 0.015 #0.001
    for i in range(len(p1)):
        p1[i].set_color('C0')
        p2[i].set_color('C1')
    myFmt = matplotlib.dates.DateFormatter('%H:%M')
    ax = plt.axes()
    ax.xaxis.set_major_formatter(myFmt)

    plt.xlim([min(times) - datetime.timedelta(minutes=10), max(times) + datetime.timedelta(minutes=10)])
    plt.xlabel('Repeat time (hh:mm)', fontsize=22, weight='bold') 
    plt.ylabel('Mean number of inliers', fontsize=22, weight='bold')
    plt.xticks(fontsize=20) 
    plt.yticks(fontsize=20) 
    plt.ylim([min(avg_inliers_segments['new2']) - 10, max(avg_inliers_segments['dark']) + 10])
    plt.title('Mean number of inliers for each repeat - on road', fontsize=22, weight='bold')

    legend_elements = [matplotlib.lines.Line2D([0], [0], color='C0', lw=4, label='Area in training data'),
                       matplotlib.lines.Line2D([0], [0], color='C1', lw=4, label='Area outside training data')]                 
    plt.legend(handles=legend_elements, fontsize=20);

    plt.savefig('{}/avg_inliers_outsidetraining_onroad.png'.format(results_dir), bbox_inches='tight', format='png')
    plt.close()

def plot_inliers(avg_inliers, times, inliers, colours, results_dir):

    # Plot average number of inliers for each run
    f = plt.figure(figsize=(30, 12))
    f.tight_layout(rect=[0, 0.03, 1, 0.95])
    p = plt.bar(times, avg_inliers, width=0.01) # 0.015 #0.001
    for i in range(len(p)):
        p[i].set_color(colours['day'][i])
    myFmt = matplotlib.dates.DateFormatter('%H:%M')
    ax = plt.axes()
    ax.xaxis.set_major_formatter(myFmt)

    plt.xlim([min(times) - datetime.timedelta(minutes=10), max(times) + datetime.timedelta(minutes=10)])
    plt.xlabel('Repeat time (hh:mm)', fontsize=22, weight='bold') 
    plt.ylabel('Mean number of inliers', fontsize=22, weight='bold')
    plt.xticks(fontsize=20) 
    plt.yticks(fontsize=20) 
    # plt.ylim([100, 450])
    # plt.ylim([40, 350])
    plt.ylim([min(avg_inliers) - 10, max(avg_inliers) + 10])
    plt.title('Mean number of inliers for each repeat', fontsize=22, weight='bold')

    # legend_elements = [matplotlib.lines.Line2D([0], [0], color='C0', lw=4, label='Day1: 03.08'),
    #                    matplotlib.lines.Line2D([0], [0], color='C1', lw=4, label='Day2: 09.08')]
    legend_elements = [matplotlib.lines.Line2D([0], [0], color='C0', lw=4, label='Day1: 15.08'),
                       matplotlib.lines.Line2D([0], [0], color='C1', lw=4, label='Day2: 16.08'),
                       matplotlib.lines.Line2D([0], [0], color='C2', lw=4, label='Day3: 20.08')]                   
    plt.legend(handles=legend_elements, fontsize=20);

    plt.savefig('{}/avg_inliers.png'.format(results_dir), bbox_inches='tight', format='png')
    plt.close()

    # f = plt.figure(figsize=(30, 12))
    # f.tight_layout(rect=[0, 0.03, 1, 0.95])
    # p = plt.bar(times, avg_inliers, width=0.001)
    # for i in range(len(p)):
    #     p[i].set_color(colours['gps'][i])
    # myFmt = matplotlib.dates.DateFormatter('%H:%M')
    # ax = plt.axes()
    # ax.xaxis.set_major_formatter(myFmt)

    # plt.xlim([min(times) - datetime.timedelta(minutes=10), max(times) + datetime.timedelta(minutes=10)])
    # plt.xlabel('Repeat time (hh:mm)', fontsize=22, weight='bold') 
    # plt.ylabel('Mean number of inliers', fontsize=22, weight='bold')
    # plt.xticks(fontsize=20) 
    # plt.yticks(fontsize=20) 
    # # plt.ylim([40, 350])
    # plt.ylim([min(avg_inliers) - 10, max(avg_inliers) + 10])
    # plt.title('Mean number of inliers for each repeat', fontsize=22, weight='bold')

    # legend_elements = [matplotlib.lines.Line2D([0], [0], color='C3', lw=4, label='GPS'),
    #                    matplotlib.lines.Line2D([0], [0], color='C2', lw=4, label='No GPS')]
    # plt.legend(handles=legend_elements, fontsize=20);

    # plt.savefig('{}/avg_inliers_gps.png'.format(results_dir), bbox_inches='tight', format='png')
    # plt.close()

    plt.rcParams['text.latex.preamble']=[r"\usepackage{lmodern}"]
    params = {'text.usetex' : True,
              'font.size' : 40,                   # Set font size to 11pt
              'axes.labelsize': 40,               # -> axis labels
              'legend.fontsize': 40,              # -> legends
              'xtick.labelsize' : 40,
              'ytick.labelsize' : 40,
              'font.family' : 'lmodern',
              'text.latex.unicode': True,
              }
    plt.rcParams.update(params) 

    # Plot cumulative distribution of inliers for each run
    fig = plt.figure(figsize=(20, 12)) #
    # fig.tight_layout(rect=[0, 0.03, 1, 0.95])
    # fig, ax = plt.subplots(figsize=(30, 20))
    # fig, axs = plt.subplots(1,2)
    # fig.tight_layout(rect=[0, 0.03, 1, 0.95])
    # fig.subplots_adjust(left=0.02, bottom=0.06, right=0.95, top=0.94, wspace=0.05)
    plot_lines = []
    labels = []
    max_inliers = 0


    noon = datetime.time(12, 0, 0)
    time_diff = []
    max_time = 0
    min_time = 0
    
    for i in range(len(inliers)):

        dateTimeA = datetime.datetime.combine(datetime.date.today(), noon)
        dateTimeB = datetime.datetime.combine(datetime.date.today(), times[i].time())
        dateTimeDifference = dateTimeA - dateTimeB
        dateTimeDifferenceInHours = abs(dateTimeDifference.total_seconds() / 3600)

        time_diff.append(dateTimeDifferenceInHours)

        if dateTimeDifferenceInHours < min_time:
            min_time = dateTimeDifferenceInHours

        if dateTimeDifferenceInHours > max_time:
            max_time = dateTimeDifferenceInHours

    time_colors = []
    for i in range(len(time_diff)):
        diff_norm = (time_diff[i] - min_time) / (max_time - min_time)
        time_colors.append(diff_norm)

    time_colors = plt.cm.viridis(time_colors)

    for i in range(len(inliers)):

        max_val = np.max(inliers[i])
        if max_val > max_inliers:
            max_inliers = max_val
        n_bins_vis_range = 50
        n_bins_total = int((n_bins_vis_range * max_val) / 696)

        values, base = np.histogram(inliers[i], bins=n_bins_total)
        unity_values = values / values.sum()
        cumulative = np.cumsum(np.flip(unity_values))
        p = plt.plot(base[:-1], 1.0 - cumulative, linewidth=5, color=time_colors[i])
        plot_lines.append(p[0])

    plt.axvline(x=6.0, color='red', linewidth='3', linestyle='--')

    cmap = matplotlib.cm.viridis
    # norm = matplotlib.colors.Normalize(vmin=0, vmax=1)

    # cbar_ax = fig.add_axes([0.09, 0.06, 0.84, 0.02])

    # cb1 = matplotlib.colorbar.ColorbarBase(cbar_ax, cmap=cmap,
    #                             norm=norm,
    #                             orientation='horizontal')

    # cax = fig.add_axes([0, 0, 0.3, 0.01]) #has to be as a list - starts with x, y coordinates for start and then width and height in % of figure width
    # cax, cbar_kwds = matplotlib.colorbar.make_axes(axs[1], location = 'right',
    #                           fraction=0.5, shrink=0.5, aspect=20)
    # norm = matplotlib.colors.Normalize(vmin =0, vmax =1)     
    # matplotlib.colorbar.ColorbarBase(cax, cmap=cmap, norm=norm, orientation='vertical')

    plt.xlim([max_inliers, 0])
    plt.ylim([0, 1])
    plt.xticks(fontsize=38)
    plt.yticks(fontsize=38)
    plt.grid(True, which='both', axis='both', color='gray', linestyle='-', 
             linewidth=1)
    plt.xlabel(r'\textbf{Number of inliers}', fontsize=50)
    plt.ylabel(r'\textbf{Cumulative distribution, keyframes}', fontsize=50)
    # plt.title(r'\textbf{Cumulative distribution of keyframes with number of inliers}', 
               # fontsize=50)
    plt.savefig('{}/inliers_cdf_exp2.png'.format(results_dir), 
                bbox_inches='tight', format='png')
    plt.close()


    fig, ax = plt.subplots(figsize=(1, 12))
    fig.subplots_adjust(bottom=0.5)

    cmap = matplotlib.cm.viridis
    norm = matplotlib.colors.Normalize(vmin=min_time, vmax=max_time)

    cb1 = matplotlib.colorbar.ColorbarBase(ax, cmap=cmap,
                                    norm=norm,
                                    orientation='vertical')
    plt.savefig('{}/colorbar.png'.format(results_dir), 
                bbox_inches='tight', format='png')
    plt.close()

def plot_data(info, path_segments, data_dir):

    avg_inliers = []
    avg_inliers_segments = {'multis':[], 'new1':[], 'dark':[], 'new2':[]}
    inliers = []
    times = [] 
    avg_comp_time = []
    colours = {'day':[], 'gps':[]}

    for i in range(len(info)):

        inliers.append(info[i]["inliers_rgb"])
        avg_inliers.append(sum(info[i]["inliers_rgb"]) / float(len(info[i]["inliers_rgb"])))
        avg_comp_time.append(sum(info[i]["comp_time"]) / float(len(info[i]["comp_time"])))

        dt = datetime.datetime.fromtimestamp(info[i]["timestamp"][0] / 1e9)	

        for key in path_segments.keys():
            avg_inliers_segments[key] += \
                            [sum(path_segments[key][i]["inliers_rgb"]) / 
                            float(len(path_segments[key][i]["inliers_rgb"]))]

        # times.append(dt.strftime('%H:%M'))

        # if i in bad_gps:
        #     colours['gps'] = colours['gps'] + ['C3']
        # else:
        #     colours['gps'] = colours['gps'] + ['C2']
        
        # if dt.day == 9:
        #     colours['day'] = colours['day'] + ['C1']
        # else:
        #     colours['day'] = colours['day'] + ['C0']

        # if dt.day != 3:
        #     dt = dt.replace(day=3)


        # if i in bad_gps:
        #     colours['gps'] = colours['gps'] + ['C3']
        # else:
        #     colours['gps'] = colours['gps'] + ['C2']
        
        if dt.day == 16:
            colours['day'] = colours['day'] + ['C1']
        elif dt.day == 15:
            colours['day'] = colours['day'] + ['C0']
        else:
            colours['day'] = colours['day'] + ['C2']

        # Cheat to get all the plot bars for one day
        if dt.day != 15:
            dt = dt.replace(day=15)
       
        times.append(dt)
        # times.append(i)

    results_dir = "{}/graph.index/repeats".format(data_dir)

    plot_inliers(avg_inliers, times, inliers, colours, results_dir)

    plot_inliers_segments(avg_inliers_segments, times, colours, results_dir)

    plot_comp_time(avg_comp_time, times, colours, results_dir)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('--path', default=None, type=str,
                        help='path to results dir (default: None)')
    parser.add_argument('--numrepeats', default=None, type=int,
                        help='number of repeats (default: None)')

    args = parser.parse_args()

    # ignore_runs = [5,6,9,10,14, 15, 16, 17, 18, 23]
    ignore_runs = [6,7,10,14,15,16,17,18,24]
    # ignore_runs = [1,2,3,4,6,31,35,39,40,10,16,21,19]
    # ignore_runs = []


    path_indices = [468, 5504, 6083, 6553, 7108]
    
    info, path_segments = load_data(args.path, args.numrepeats, ignore_runs, path_indices)

    plot_data(info, path_segments, args.path);


