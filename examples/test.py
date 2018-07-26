#! /usr/bin/python3


import sys
sys.path.insert(0, '../')

from python.echmetupdatecheck import ECHMETUpdateCheck


def main():
    updater = ECHMETUpdateCheck('../build/libECHMETUpdateCheck.so')
    url = 'https://devoid-pointer.net/misc/eupd_sample_list.txt'

    ret = updater.check(url,
                        ECHMETUpdateCheck.Software('armageddon architect',
                                                   ECHMETUpdateCheck.Version(0, 0, '')),
                        False)

    if ret[0] is False:
        print('Update check failed: {}'.format(updater.error_to_str(ret[1])))
    else:
        print(updater.error_to_str(ret[1]))
        print(ret[2])

    ret = updater.check_many(url,
                             [ECHMETUpdateCheck.Software('Doomsday machine',
                                                         ECHMETUpdateCheck.Version(1, 0, 'a')),
                              ECHMETUpdateCheck.Software('microsoft windows',
                                                         ECHMETUpdateCheck.Version(10, 1701, 'a'))

                              ],
                             False)
    if ret[0] is False:
        print('Failed to check for updates: {}'.format(updater.error_to_str(ret[1])))
    else:
        for item in ret[2]:
            print(item[0], item[1])


if __name__ == '__main__':
    main()
