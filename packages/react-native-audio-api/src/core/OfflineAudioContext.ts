import { IOfflineAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import { OfflineAudioContextOptions } from '../types';
import { InvalidStateError, NotSupportedError } from '../errors';
import AudioBuffer from './AudioBuffer';
import { isWorkletsAvailable, workletsModule } from '../utils';

export default class OfflineAudioContext extends BaseAudioContext {
  private isSuspended: boolean;
  private isRendering: boolean;
  private duration: number;

  // We need to keep here a reference to this runtime to better manage its lifecycle
  // eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-explicit-any
  private _audioRuntime: any;

  constructor(options: OfflineAudioContextOptions);
  constructor(numberOfChannels: number, length: number, sampleRate: number);
  constructor(
    arg0: OfflineAudioContextOptions | number,
    arg1?: number,
    arg2?: number
  ) {
    let audioRuntime = null;
    if (isWorkletsAvailable) {
      audioRuntime = workletsModule.createWorkletRuntime('AudioWorkletRuntime');
    }

    if (typeof arg0 === 'object') {
      const { numberOfChannels, length, sampleRate } = arg0;
      super(
        global.createOfflineAudioContext(
          numberOfChannels,
          length,
          sampleRate,
          audioRuntime
        )
      );

      this.duration = length / sampleRate;
    } else if (
      typeof arg0 === 'number' &&
      typeof arg1 === 'number' &&
      typeof arg2 === 'number'
    ) {
      super(global.createOfflineAudioContext(arg0, arg1, arg2, audioRuntime));
      this.duration = arg1 / arg2;
    } else {
      throw new NotSupportedError('Invalid constructor arguments');
    }

    this.isSuspended = false;
    this.isRendering = false;
    this._audioRuntime = audioRuntime;
  }

  async resume(): Promise<undefined> {
    if (!this.isRendering) {
      throw new InvalidStateError(
        'Cannot resume an OfflineAudioContext while rendering'
      );
    }

    if (!this.isSuspended) {
      throw new InvalidStateError(
        'Cannot resume an OfflineAudioContext that is not suspended'
      );
    }

    this.isSuspended = false;

    await (this.context as IOfflineAudioContext).resume();
  }

  async suspend(suspendTime: number): Promise<undefined> {
    if (suspendTime < 0) {
      throw new InvalidStateError('suspendTime must be a non-negative number');
    }

    if (suspendTime < this.context.currentTime) {
      throw new InvalidStateError(
        `suspendTime must be greater than the current time: ${suspendTime}`
      );
    }

    if (suspendTime > this.duration) {
      throw new InvalidStateError(
        `suspendTime must be less than the duration of the context: ${suspendTime}`
      );
    }

    this.isSuspended = true;

    await (this.context as IOfflineAudioContext).suspend(suspendTime);
  }

  async startRendering(): Promise<AudioBuffer> {
    if (this.isRendering) {
      throw new InvalidStateError('OfflineAudioContext is already rendering');
    }

    this.isRendering = true;

    const audioBuffer = await (
      this.context as IOfflineAudioContext
    ).startRendering();

    return new AudioBuffer(audioBuffer);
  }
}
