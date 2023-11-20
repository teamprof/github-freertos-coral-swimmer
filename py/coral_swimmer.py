#!/usr/bin/python3
import sys
import os

# import time

# import argparse
# import base64
import cv2
# import eel
import numpy as np
# import os
import rpc_helper
# from PIL import Image

# import argparse
import base64
import requests
from PIL import Image, ImageDraw
import matplotlib.pyplot as plt


LaneRopeRegionX = 0.75/2.5


class Config:
    host_ip = '10.10.10.1'
    width = 324
    height = 324
    format = 'RGB'
    filter = 'BILINEAR'
    rotation = 0
    auto_white_balance = True


def get_field_or_die(data, field_name):
    if field_name not in data:
        print(f'Unable to parse {field_name} from data: {data}\r\n')
        exit(1)
    return data[field_name]


# def createWin(config):
#     windowTitle = 'Swimmer assistive system'
#     arr = np.zeros((config.width, config.height))
#     img = Image.fromarray(arr, config.format)
#     # draw = ImageDraw.Draw(img)
#     img_cv = cv2.cvtColor(np.array(img), cv2.COLOR_RGB2BGR)
#     cv2.imshow(windowTitle, img_cv)
#     # cv2.namedWindow(windowTitle, cv2.WINDOW_NORMAL)
#     # cv2.resizeWindow(windowTitle, 480, 480)


def getResponse(config):
    response = rpc_helper.get_image_from_camera(
        config.host_ip,
        config.width, config.height,
        config.format, config.filter, config.rotation, config.auto_white_balance)
    return response


def loadLabels():
    path = os.path.dirname(__file__)
    fname = os.path.join(path, "coco_labels.txt")
    with open(fname, 'r') as file:
        data = file.read().splitlines()
    return data


def main():
    config = Config()

    labels = loadLabels()
    # print(labels)
    print('len(labels):', len(labels))

    windowTitle = 'Swimmer assistive system'
    # arr = np.zeros((config.width, config.height))
    # img = Image.fromarray(arr, config.format)
    # # draw = ImageDraw.Draw(img)
    # img_cv = cv2.cvtColor(np.array(img), cv2.COLOR_RGB2BGR)
    # cv2.imshow(windowTitle, img_cv)
    # cv2.namedWindow(windowTitle, cv2.WINDOW_NORMAL)
    # cv2.resizeWindow(windowTitle, 480, 480)

    # createWin(config)

    # cv2.startWindowThread()
    # cv2.namedWindow('Swimmer assistive system', cv2.WND_PROP_AUTOSIZE,)
    # cv2.namedWindow(windowTitle, cv2.WINDOW_NORMAL)
    # cv2.resizeWindow(windowTitle, 480, 480)

    while True:
        response = getResponse(config)

        result = get_field_or_die(response, 'result')
        width = get_field_or_die(result, 'width')
        height = get_field_or_die(result, 'height')

        image_data_base64 = get_field_or_die(result, 'base64_data')
        image_data = base64.b64decode(image_data_base64)
        img = Image.frombytes('RGB', (width, height), image_data, 'raw')
        draw = ImageDraw.Draw(img)

        tfObjects = get_field_or_die(result, 'tfObjects')
        # print('len(tfObjects)=', len(tfObjects))
        for tfObj in tfObjects:
            id = get_field_or_die(tfObj, 'id')
            score = get_field_or_die(tfObj, 'score')
            left = get_field_or_die(tfObj, 'xmin') * width
            top = get_field_or_die(tfObj, 'ymin') * height
            right = get_field_or_die(tfObj, 'xmax') * width
            bottom = get_field_or_die(tfObj, 'ymax') * height
            draw.rectangle([left, top, right, bottom])

            text = f'ID: {labels[id]}' if id < len(labels) else f'ID: {id}'
            text = text + f' Score: {score}'
            # text = f'ID: {id} Score: {score}'
            draw.text((left, bottom), text)
            # print('id=', id, ', score =', score, ', xmin=', xmin,
            #       ', xmax=', xmax, ', ymin=', ymin, 'ymax=', ymax)

        tfSwimmers = get_field_or_die(result, 'tfSwimmers')
        tfLanes = get_field_or_die(result, 'tfLanes')
        if len(tfLanes) > 0:
            tfLane = tfLanes[0]
            colorRed = (255, 0, 0)
            colorGreen = (0, 255, 0)
            colorBlue = (0, 0, 255)
            thickness = 2

            top = 0.0
            bottom = height

            left = get_field_or_die(tfLane, 'xmin') * width
            draw.line([left, top, left, bottom], colorBlue)

            right = get_field_or_die(tfLane, 'xmax') * width
            draw.line([right, top, right, bottom], colorBlue)

            # print(f'left={left}, right={right}')
            # print(f'tfSwimmers[0]={tfSwimmers[0]}')

            if len(tfSwimmers) > 0:
                tfSwimmer = tfSwimmers[0]
                x = (tfSwimmer['xmin'] + tfSwimmer['xmax']) / 2

                laneXmin = get_field_or_die(tfLane, 'xmin')
                laneXmax = get_field_or_die(tfLane, 'xmax')
                laneYmin = get_field_or_die(tfLane, 'ymin')
                laneYmax = get_field_or_die(tfLane, 'ymax')

                regionBoundL = laneXmin + LaneRopeRegionX
                regionBoundR = laneXmax - LaneRopeRegionX
                regionBoundT = laneYmin * height
                regionBoundB = laneYmax * height
                # print(
                #     f'x={x}, laneXmin={laneXmin}, laneXmax={laneXmax}, regionBoundL={regionBoundL}, regionBoundR={regionBoundR}')

                if x < regionBoundL:
                    draw.rectangle([left, regionBoundT, regionBoundL*width, regionBoundB],
                                   None, colorRed, thickness)
                    # draw.rectangle([left, top, regionBoundL*width, bottom],
                    #                colorRed)
                elif x > regionBoundR:
                    draw.rectangle([regionBoundR*width, regionBoundT, right, regionBoundB],
                                   None, colorRed, thickness)
                    # draw.rectangle([regionBoundR*width, top, right, bottom],
                    #                colorRed)
                else:
                    draw.rectangle([regionBoundL*width, regionBoundT, regionBoundR*width, regionBoundB],
                                   None, colorGreen, thickness)
                    # draw.rectangle([regionBoundL*width, top, regionBoundR*width, bottom],
                    #                colorGreen)

        img_cv = cv2.cvtColor(np.array(img), cv2.COLOR_RGB2BGR)
        cv2.imshow(windowTitle, img_cv)
        cv2.namedWindow(windowTitle, cv2.WINDOW_NORMAL)
        cv2.resizeWindow(windowTitle, 480, 640)

        key = cv2.waitKey(200)
        # key = cv2.waitKey(1000)
        if key == ord('q'):
            break

    cv2.destroyAllWindows()


if __name__ == "__main__":
    try:
        main()
    except requests.exceptions.ConnectionError:
        msg = 'ERROR: Cannot connect to Coral Dev Board Micro, make sure you specify the correct IP address with --host.'
        if sys.platform == 'darwin':
            msg += ' Network over USB is not supported on macOS.'
        print(msg, file=sys.stderr)
