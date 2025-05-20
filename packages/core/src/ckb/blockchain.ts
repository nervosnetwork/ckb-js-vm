import { Bytes, BytesLike } from "../bytes/index";
import { mol } from "../molecule/index";
import { Num, NumLike } from "../num/index";

/**
 * @public
 */
export type RawHeaderLike = {
  version: NumLike;
  compact_target: NumLike;
  timestamp: NumLike;
  number: NumLike;
  epoch: NumLike;
  parent_hash: BytesLike;
  transactions_root: BytesLike;
  proposals_hash: BytesLike;
  extra_hash: BytesLike;
  dao: BytesLike;
};

/**
 * @public
 */
@mol.codec(
  mol.struct({
    version: mol.Uint32,
    compact_target: mol.Uint32,
    timestamp: mol.Uint64,
    number: mol.Uint64,
    epoch: mol.Uint64,
    parent_hash: mol.Byte32,
    transactions_root: mol.Byte32,
    proposals_hash: mol.Byte32,
    extra_hash: mol.Byte32,
    dao: mol.Byte32,
  }),
)
export class RawHeader extends mol.Entity.Base<RawHeaderLike, RawHeader>() {
  /**
   * Creates an instance of RawHeader.
   *
   * @param version - The version of the block.
   * @param compact_target - The difficulty of the PoW puzzle represented in compact target format.
   * @param timestamp - A Unix time timestamp in milliseconds.
   * @param number - The block height.
   * @param epoch - Current epoch information.
   * @param parent_hash - The hash of the parent block.
   * @param transactions_root - The hash of concatenated transaction hashes CBMT root and transaction witness hashes CBMT root.
   * @param proposals_hash - The hash of concatenated proposal ids. (all zeros when proposals is empty)
   * @param extra_hash - The hash of concatenated hashes of uncle block headers. （all zeros when uncles is empty)
   * @param dao - Data containing DAO related information. Please refer to Nervos DAO RFC for details on this field.
   */

  constructor(
    public version: NumLike,
    public compact_target: NumLike,
    public timestamp: NumLike,
    public number: NumLike,
    public epoch: NumLike,
    public parent_hash: Bytes,
    public transactions_root: Bytes,
    public proposals_hash: Bytes,
    public extra_hash: Bytes,
    public dao: Bytes,
  ) {
    super();
  }

  /**
   * Creates an RawHeader instance from an RawHeaderLike object.
   *
   * @param rawHeader - An RawHeaderLike object or an instance of RawHeader.
   * @returns An RawHeader instance.
   *
   * @example
   * ```typescript
   * const rawHeader = RawPoint.from({ version: 0, ... dao: "0x..." });
   * ```
   */
  static from(rawHeader: RawHeaderLike): RawHeader {
    if (rawHeader instanceof RawHeader) {
      return rawHeader;
    }
    return new RawHeader(
      rawHeader.version,
      rawHeader.compact_target,
      rawHeader.timestamp,
      rawHeader.number,
      rawHeader.epoch,
      rawHeader.parent_hash,
      rawHeader.transactions_root,
      rawHeader.proposals_hash,
      rawHeader.extra_hash,
      rawHeader.dao,
    );
  }
}

/**
 * @public
 */
export type HeaderLike = {
  rawHeader: RawHeaderLike;
  nonce: bigint;
};

/**
 * @public
 */
@mol.codec(
  mol.struct({
    rawHeader: RawHeader,
    nonce: mol.Uint128,
  }),
)
export class Header extends mol.Entity.Base<HeaderLike, Header>() {
  /**
   * Creates an instance of RawHeader.
   *
   * @param rawHeader - The raw header.
   * @param nonce - The nonce. Similar to the nonce in Bitcoin. Represent the solution of the PoW puzzle.
   */

  constructor(
    public rawHeader: RawHeader,
    public nonce: bigint,
  ) {
    super();
  }

  /**
   * Creates an Header instance from an HeaderLike object.
   *
   * @param header - A HeaderLike object or an instance of Header.
   * @returns A Header instance.
   *
   * @example
   * ```typescript
   * const header = Header.from({ rawHeader: {}, nonce: 1 });
   * ```
   */
  static from(header: HeaderLike): Header {
    if (header instanceof Header) {
      return header;
    }
    return new Header(RawHeader.from(header.rawHeader), header.nonce);
  }
}
